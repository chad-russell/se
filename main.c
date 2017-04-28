#include <stdio.h>
#include <stdlib.h>

#include <czmq.h>

#include "forward_types.h"
#include "rope.h"
#include "util.h"
#include "buf.h"
#include "circular_buffer.h"
#include "json_tokenize.h"

#define UNDO_BUFFER_SIZE 10

//struct editor_buffer_t
//read_file(const char *file_name)
//{
//    FILE *file = fopen(file_name, "r");
//    if (file == NULL) {
//        fprintf(stderr, "error opening file\n");
//        exit(-1);
//    }
//
//    int err;
//
//    err = fseek(file, 0, SEEK_END);
//    if (err == -1) {
//        fprintf(stderr, "fseek failed\n");
//    }
//
//    int64_t file_size = ftell(file);
//    if (file_size == -1) {
//        fprintf(stderr, "error determining size of file\n");
//    }
//
//    rewind(file);
//
//    char *src = (char *) se_alloc(file_size + 1, sizeof(char));
//    if (src == NULL) {
//        fprintf(stderr, "could not allocate for reading file into memory\n");
//    }
//
//    size_t bytes_read = fread(src, sizeof(char), (size_t) file_size, file);
//    if (bytes_read != file_size) {
//        fprintf(stderr, "error reading file\n");
//        exit(-1);
//    }
//
//    src[file_size] = '\0';
//
//    err = fclose(file);
//    if (err != 0) {
//        fprintf(stderr, "warning: failure closing file\n");
//    }
//
//    struct editor_buffer_t ctx;
//    ctx.undo_buffer = circular_buffer_init(sizeof(struct editor_screen_t), UNDO_BUFFER_SIZE);
//
//    struct editor_screen_t screen;
//
//    screen.rn = rope_leaf_init(src);
//
//    struct cursor_info_t cursor_info;
//    cursor_info.cursor_pos = 0;
//    cursor_info.cursor_byte_pos = 0;
//    cursor_info.cursor_row = 0;
//    cursor_info.cursor_col = 0;
//    screen.cursor_info = cursor_info;
//
//    undo_stack_append(ctx, screen);
//
//    return ctx;
//}
//
//static void
//echo_actor(zsock_t *pipe, void *args)
//{
//    // Actor must call zsock_signal(pipe, 0) when initialized
//    zsock_signal(pipe, 0);
//
//    bool terminated = false;
//    while (!terminated) {
//        zmsg_t *msg = zmsg_recv(pipe);
//        if (!msg) {
//            // Interrupted
//            break;
//        }
//
//        char *command = zmsg_popstr(msg);
//
//        // All actors must handle $TERM in this way
//        if (streq(command, "$TERM")) {
//            terminated = true;
//        }
//        // This is an example command for our test actor
//        else if (streq(command, "ECHO")) {
////            zmsg_send(&msg, pipe);
//        }
//        else {
//            puts("E: invalid message to actor");
//            assert(false);
//        }
//
//        free(command);
//        zmsg_destroy(&msg);
//    }
//}
//
//void
//zactor_example()
//{
//    zactor_t *actor = zactor_new(echo_actor, NULL);
//
//    while (!zsys_interrupted) {
//        zstr_sendx(actor, "ECHO", "This is a string", NULL);
//        char *string = zstr_recv(actor);
//        printf("string: %s\n", string);
//        free(string);
//    }
//
//    printf("interrupt received... cleaning up...\n");
//    zactor_destroy(&actor);
//}

void
test_scratch(struct editor_buffer_t ctx, struct editor_screen_t screen)
{
    struct rope_t *rn = screen.rn;
    for (int i = 0; i < 15; i++) {
        rn = rope_append(rn, "a");
        screen.rn = rn;

        undo_stack_append(ctx, screen);
    }
}

void
run_main_zmq_loop(struct editor_screen_t screen, struct editor_buffer_t ctx)
{
    // todo(chad):
    // get undo_stack working in a way that we can easily get the last element
    // as well as pop the last element
    struct rope_t *last_rn = screen.rn;

    // keep track of where we are in the undo cycle, so we know where to append
    int64_t undo_idx = -1;

    zsock_t *rep = zsock_new_rep("tcp://*:5555");
    while (!zsys_interrupted) {
        char *msg = zstr_recv(rep);

//        buf_print_fmt("received: %str\n", msg);

        struct json_parser_t p;
        json_parser_init(&p);

        struct json_tok_t t[512];
        uint32_t r = json_parser_parse(&p, msg, strlen(msg), t, sizeof(t)/sizeof(t[0]));

        if (r > 0) {
            SE_ASSERT_MSG(t[0].type == JSON_TOK_TYPE_OBJECT, "expected t[0].type == object");
            SE_ASSERT_MSG(t[1].type == JSON_TOK_TYPE_STRING, "expected t[1].type == string");
            SE_ASSERT_MSG(t[2].type == JSON_TOK_TYPE_STRING, "expected t[2].type == string");

            if (!strncmp(msg + t[2].start, "backspace", t[2].end - t[2].start)) {
                int64_t end = rope_total_char_length(last_rn);
                if (end > 0) {
                    last_rn = rope_delete(last_rn, end - 1, end);
                    screen.rn = last_rn;

                    if (undo_idx != -1) {
                        circular_buffer_truncate(ctx.undo_buffer, undo_idx);
                        undo_idx = -1;
                    }
                    undo_stack_append(ctx, screen);
                }
            } else if (!strncmp(msg + t[2].start, "append", t[2].end - t[2].start)) {
                // todo(chad): this doesn't take escapes into account (e.g. '\n', '\r', '\"', etc.)
                struct buf_t *buf = buf_init_fmt("%n_str", t[4].end - t[4].start, msg + t[4].start);

                last_rn = rope_append(last_rn, buf->bytes);
                screen.rn = last_rn;

                if (undo_idx != -1) {
                    circular_buffer_truncate(ctx.undo_buffer, undo_idx);
                    undo_idx = -1;
                }
                undo_stack_append(ctx, screen);

                buf_free(buf);
            } else if (!strncmp(msg + t[2].start, "undo", t[2].end - t[2].start)) {
                struct buf_t *buf = buf_init_fmt("%n_str", t[4].end - t[4].start, msg + t[4].start);
                sscanf(buf->bytes, "%lli", &undo_idx);
                buf_free(buf);

                struct editor_screen_t *undone_screen = circular_buffer_at(ctx.undo_buffer, undo_idx);
                last_rn = undone_screen->rn;
            } else {
                SE_PANIC("unknown request");
            }
        }

        // todo(chad): %rope should serialize with escapes (or probably make a new %rope_escaped)
        struct buf_t *to_send = buf_init_fmt("{\"undo_length\": %i64, \"text\": \"%rope\"}",
                                             ctx.undo_buffer->length,
                                             last_rn);

        zstr_send(rep, to_send->bytes);
        buf_free(to_send);
        zstr_free(&msg);
    }
}

int
main()
{
    struct editor_buffer_t ctx;
    ctx.undo_buffer = circular_buffer_init(sizeof(struct editor_screen_t), UNDO_BUFFER_SIZE);

    struct editor_screen_t screen;
    screen.rn = rope_leaf_init("");
    screen.cursor_info.cursor_pos = 0;
    screen.cursor_info.cursor_byte_pos = 0;
    screen.cursor_info.cursor_row = 0;
    screen.cursor_info.cursor_col = 0;
    undo_stack_append(ctx, screen);

//    test_scratch(ctx, screen);
//    zactor_example();
    run_main_zmq_loop(screen, ctx);

    return 0;
}
