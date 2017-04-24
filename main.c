#include <stdio.h>
#include <stdlib.h>

#include <czmq.h>

#include "all_types.h"
#include "rope.h"
#include "util.h"
#include "buf.h"

#define UNDO_BUFFER_SIZE 10

struct editor_buffer_t
read_file(const char *file_name)
{
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        fprintf(stderr, "error opening file\n");
        exit(-1);
    }

    int err;

    err = fseek(file, 0, SEEK_END);
    if (err == -1) {
        fprintf(stderr, "fseek failed\n");
    }

    size_t file_size = (size_t) ftell(file);
    if (file_size == -1) {
        fprintf(stderr, "error determining size of file\n");
    }

    rewind(file);

    char *src = (char *) se_calloc(file_size + 1, sizeof(char));
    if (src == NULL) {
        fprintf(stderr, "could not allocate for reading file into memory\n");
    }

    size_t bytes_read = fread(src, sizeof(char), file_size, file);
    if (bytes_read != file_size) {
        fprintf(stderr, "error reading file\n");
        exit(-1);
    }

    src[file_size] = '\0';

    err = fclose(file);
    if (err != 0) {
        fprintf(stderr, "warning: failure closing file\n");
    }

    struct editor_buffer_t ctx;
    ctx.undo_buffer = circular_buffer_init(sizeof(struct editor_screen_t), UNDO_BUFFER_SIZE);

    undo_stack_append(ctx, rope_leaf_new(src, ctx));

    return ctx;
}

static void
echo_actor(zsock_t *pipe, void *args)
{
    // Actor must call zsock_signal(pipe, 0) when initialized
    zsock_signal(pipe, 0);

    bool terminated = false;
    while (!terminated) {
        zmsg_t *msg = zmsg_recv(pipe);
        if (!msg) {
            // Interrupted
            break;
        }

        char *command = zmsg_popstr(msg);

        // All actors must handle $TERM in this way
        if (streq(command, "$TERM")) {
            terminated = true;
        }
        // This is an example command for our test actor
        else if (streq(command, "ECHO")) {
            zmsg_send(&msg, pipe);
        }
        else {
            puts("E: invalid message to actor");
            assert(false);
        }

        free(command);
        zmsg_destroy(&msg);
    }
}

void
zactor_example()
{
    zactor_t *actor = zactor_new(echo_actor, NULL);

    while (!zsys_interrupted) {
        zstr_sendx(actor, "ECHO", "This is a string", NULL);
        char *string = zstr_recv(actor);
        printf("string: %s\n", string);
        free(string);
    }

    printf("interrupt received... cleaning up...\n");
    zactor_destroy(&actor);
}

void
run_main_zmq_loop(struct editor_screen_t screen, struct editor_buffer_t ctx)
{
    // todo(chad):
    // get undo_stack working in a way that we can easily get the last element
    // as well as pop the last element
    struct rope_t *last_rn = screen.rn;

    zsock_t *rep = zsock_new_rep("tcp://*:5555");
    while (!zsys_interrupted) {
        char *msg = zstr_recv(rep);
//        printf("received \"%s\"\n", msg);

//        struct rope_t *saved = last_rn;

        if (!strcmp(msg, "BACKSPACE")) {
            int64_t end = rope_total_char_length(last_rn);
            if (end > 0) {
                last_rn = rope_delete(last_rn, end - 1, end, ctx);
                undo_stack_append(ctx, last_rn);
//                rope_free(saved);
            }
        } else {
            last_rn = rope_append(last_rn, msg, ctx);
            undo_stack_append(ctx, last_rn);
//            rope_free(saved);
        }

        struct buf_t *to_send = buf_init_fmt("%rope", last_rn);

        zstr_send(rep, to_send->bytes);
        buf_free(to_send);
        zstr_free(&msg);
    }
}

void
test_append_delete(struct rope_t *rn, struct editor_buffer_t ctx)
{
    struct rope_t *saved;

    while (1) {
        // append
        saved = rn;
        rn = rope_append(rn, "a", ctx);
        rope_free(saved);

        // delete
        saved = rn;
        int64_t end = rope_total_char_length(rn);
        if (end > 0) {
            rn = rope_delete(rn, end - 1, end, ctx);
            rope_free(saved);
        }
    }
}

int
main()
{
    struct editor_buffer_t ctx;
    ctx.undo_buffer = circular_buffer_init(sizeof(struct editor_screen_t), UNDO_BUFFER_SIZE);

    struct editor_screen_t screen;
    screen.rn = rope_leaf_new("", ctx);
    screen.cursor_info.cursor_pos = 0;
    screen.cursor_info.cursor_byte_pos = 0;
    screen.cursor_info.cursor_row = 0;
    screen.cursor_info.cursor_col = 0;
    circular_buffer_append(ctx.undo_buffer, &screen);

//    test_append_delete(screen.rn, ctx);
    run_main_zmq_loop(screen, ctx);

    return 0;
}
