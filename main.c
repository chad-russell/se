#include <stdio.h>
#include <stdlib.h>

#include <czmq.h>

#include "all_types.h"
#include "rope_node.h"

void
test_rope_operations()
{
    struct editor_context_t ctx;
    ctx.undo_stack = vector_init(1, sizeof(struct editor_screen_t));

    struct rope_node_t *hello = rope_node_leaf_new("hello,_my_name_is_", ctx);
//    hello->rc += 1;

    struct rope_node_t *deleted = rope_node_delete(hello, 5, 9, ctx);
    deleted->rc += 1;

    struct rope_node_t *inserted = rope_node_insert(hello, 18, "Chad!!", ctx);
//    inserted->rc += 1;

    rope_node_free(inserted);
    rope_node_free(hello);
    rope_node_print(deleted);
}

struct editor_context_t
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

    char *src = (char *) calloc(file_size + 1, sizeof(char));
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

    struct editor_context_t ctx;
    ctx.undo_stack = vector_init(1, sizeof(struct editor_screen_t));

    struct editor_screen_t editor_screen;
    editor_screen.cursor_pos = 0;
    editor_screen.cursor_byte_pos = 0;
    editor_screen.cursor_row = 0;
    editor_screen.cursor_col = 0;
    editor_screen.root = rope_node_leaf_new(src, ctx);
    editor_screen.root->rc += 1; // retain this for the time being

    vector_append(ctx.undo_stack, &editor_screen);

    return ctx;
}

//  --------------------------------------------------------------------------
//  Actor
//  must call zsock_signal(pipe, 0) when initialized
//  must listen to pipe and exit on $TERM command

static void
echo_actor (zsock_t *pipe, void *args)
{
    //  Do some initialization
    assert(streq((char *) args, "Hello, World"));
    zsock_signal (pipe, 0);

    bool terminated = false;
    while (!terminated) {
        zmsg_t *msg = zmsg_recv(pipe);
        if (!msg)
            break;              //  Interrupted
        char *command = zmsg_popstr (msg);
        //  All actors must handle $TERM in this way
        if (streq(command, "$TERM")) {
            terminated = true;
        }
        //  This is an example command for our test actor
        else if (streq(command, "ECHO")) {
            zmsg_send(&msg, pipe);
        }
        else {
            puts("E: invalid message to actor");
            assert (false);
        }
        free(command);
        zmsg_destroy (&msg);
    }
}

void
zactor_example()
{
    zactor_t *actor = zactor_new(echo_actor, "Hello, World");

    while (!zctx_interrupted) {
        zstr_sendx(actor, "ECHO", "This is a string", NULL);
        char *string = zstr_recv(actor);
        printf("string: %s\n", string);
        free(string);
    }

    printf("interrupt received... cleaning up...\n");
    zactor_destroy(&actor);
}

int
main()
{
//    test_rope_operations();
    struct editor_context_t ctx = read_file("/Users/chadrussell/Projects/text/hello.txt");
    struct editor_screen_t *screen = (struct editor_screen_t *) ctx.undo_stack->buf;
    struct rope_node_t *inserted = rope_node_insert(screen->root, 48, "***", ctx);
    rope_node_print(inserted);

//    zactor_example();

    return 0;
}
