#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct header {

    char magic1;
    char magic2;
    char opcode;
    char payload_len;

    uint32_t token;
    uint32_t msg_id;
};

const int h_size = sizeof(struct header);

// These are the constants indicating the states.
#define STATE_OFFLINE          0
#define STATE_LOGIN_SENT       1
#define STATE_ONLINE           2
// Now you can define other states in a similar fashion.

// These are the constants indicating the events.
// All events starting with EVENT_USER_ are generated by a human user.
#define EVENT_USER_LOGIN               0
#define EVENT_USER_POST                1
// Now you can define other events from the user.
......
#define EVENT_USER_INVALID             79

// All events starting with EVENT_NET_ are generated by receiving a msg
// from the network. We deliberately use a larger numbers to help debug.
#define EVENT_NET_LOGIN_SUCCESSFUL      80
#define EVENT_NET_POST_ACK              81
// Now you can define other events from the network.
......
#define EVENT_NET_INVALID               255

// These are the constants indicating the opcodes.
#define OPCODE_RESET                    0x00
#define OPCODE_MUST_LOGIN_FIRST_ERROR   0xF0
#define OPCODE_LOGIN                    0x10
// Now you can define other opcodes in a similar fashion.
......

int main() {

    char user_input[1024];

    int ret;
    int sockfd = 0;
    char send_buffer[1024];
    char recv_buffer[1024];
    struct sockaddr_in serv_addr;
    struct sockaddr_in my_addr;
    int maxfd;
    fd_set read_set;
    FD_ZERO(&read_set);


    // You just need one socket file descriptor. I made a mistake previously
    // and defined two socket file descriptors.
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
    printf("socket() error: %s.\n", strerror(errno));
        return -1;
    }

    // The "serv_addr" is the server's address and port number, 
    // i.e, the destination address if the client needs to send something. 
    // Note that this "serv_addr" must match with the address in the 
    // "UDP receive" code.
    // We assume the server is also running on the same machine, and 
    // hence, the IP address of the server is just "127.0.0.1".
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(32000);

    // The "my_addr" is the client's address and port number used for  
    // receiving responses from the server.
    // Note that this is a local address, not a remote address.
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    my_addr.sin_port = htons(some_semi_random_port_number);


    // Bind "my_addr" to the socket for receiving messages from the server.
    bind(sockfd, 
         (struct sockaddr *) &my_addr, 
         sizeof(my_addr));

    maxfd = sockfd + 1; // Note that the file descriptor of stdin is "0"
    
    int state = STATE_OFFLINE;
    int event;
    uint32_t token; // Assume the token is a 32-bit integer

    // This is a pointer of the type "struct header" but it always points
    // to the first byte of the "send_buffer", i.e., if we dereference this
    // pointer, we get the first 12 bytes in the "send_buffer" in the format
    // of the structure, which is very convenient.
    struct header *ph_send = (struct header *)send_buffer;
    // So as the receive buffer.
    struct header *ph_recv = (struct header *)recv_buffer;

    while (1) {
    
        // Use select to wait on keyboard input or socket receiving.
        FD_SET(fileno(stdin), &read_set);
        FD_SET(sockfd, &read_set);

        select(maxfd, &read_set, NULL, NULL, NULL);

        if (FD_ISSET(fileno(stdin), &read_set)) {

            // Now we know there is a keyboard input event
            // TODO: Figure out which event and process it according to the
            // current state

            fgets(user_input, sizeof(user_input), stdin);

            // Note that in this parse function, you need to check the
            // user input and figure out what event it is. Basically it
            // will be a long sequence of if (strncmp(user_input, ...) == 0)
            // and if none of the "if" matches, return EVENT_USER_INVALID
            event = parse_the_event_from_the_input_string(...)

            // You can also add a line to print the "event" for debugging.

        if (event == EVENT_USER_LOGIN) {
                    if (state == STATE_OFFLINE) {

                    // CAUTION: we do not need to parse the user ID and
                    // and password string, assuming they are always in the
                    // correct format. The server will parse it anyway.

                    char *id_password = user_input + 6 // skip the "login#"
                    int m = strlen(id_password);

                    ph_send->magic1 = MAGIC_1;
                    ph_send->magic2 = MAGIC_2;
                    ph_send->opcode = OPCODE_LOGIN;
                    ph_send->payload_len = m;
                    ph_send->token = 0;
                    ph_send->msg_id = 0;

                    memcpy(send_buffer + h_size, id_password, m);

                    sendto(sockfd, send_buffer, h_size + m, 0, 
                        (struct sockaddr *) &serv_addr, sizeof(serv_add));

                    // Once the corresponding action finishes, transit to
                    // the login_sent state
                        state = LOGIN_SENT;

                } else {

                    // TODO: handle errors if the event happens in a state
                    // that is not expected. Basically just print an error
                    // message and doing nothing. Note that if a user types
                    // something invalid, it does not need to trigger a 
                    // session reset.

                }

            } else if (event == EVENT_USER_POST) {

                    // Note that this is similar to the login msg.
                    // Actually, these messages are carefully designed to 
                    // somewhat minimize the processing on the client side.
                    // If you look at the "subscribe", "unsubscribe", "post"
                    // and "retrieve", they are all similar, i.e., just fill
                    // the header and copy the user input after the "#" as
                    // the payload of the message, then just send the msg.

                    char *text = user_input + 5 // skip the "post#"
                    int m = strlen(text);

                    ph_send->magic1 = MAGIC_1;
                    ph_send->magic2 = MAGIC_2;
                    ph_send->opcode = OPCODE_POST;
                    ph_send->payload_len = m;
                    ph_send->token = token;
                    ph_send->msg_id = 0;

                    memcpy(send_buffer + h_size, text, m);

                    sendto(sockfd, send_buffer, h_size + m, 0, 
                        (struct sockaddr *) &serv_addr, sizeof(serv_add));


            } else if (event == EVENT_USER_RESET) {

                // TODO: You may add another command like "reset#" so as to
                // facilitate testing. In this case, a user just need to 
                // type this line to generate a reset message.

                // You can add more command as you like to help debugging.
                // For example, I can add a command "state#" to instruct the
                // client program to print the current state without chang
                // -ing anything.


            } else if (event == .../* some other event */) {

                // TODO: process other event

            }

        }
        if (FD_ISSET(sockfd, &read_set)) {

            // Now we know there is an event from the network
            // TODO: Figure out which event and process it according to the
            // current state

            ret = recv(sockfd, recv_buffer, sizeof(recv_buffer), 0);

            event = parse_the_event_from_the_received_message(...)

            if (event == EVENT_NET_LOGIN_SUCCESSFUL) {
                    if (state == STATE_LOGIN_SENT) {

                    token = ph_recv->token;

                    // TODO: print a line of "login_ack#successful"
                    state = STATE_ONLINE;

                } else {

                    // A spurious msg is received. Just reset the session.
                    // You can define a function "send_reset()" for 
                    // convenience because it might be used in many places.
                    send_reset(sockfd, send_buffer);

                    state = STATE_OFFLINE;
                }
            } else if (event == EVENT_NET_LOGIN_FAILED) {
                    if (state == STATE_LOGIN_SENT) {

                    // TODO: print a line of "login_ack#failed"
                    state = STATE_OFFLINE;

                } else {

                    send_reset(sockfd, send_buffer);

                    state = STATE_OFFLINE;
                }

            } else if (event == EVENT_NET_FORWARD) {
                    if (state == STATE_ONLINE) {

                    // Just extract the payload and print the text.
                    char *text = recv_buffer + h_size;

                    printf("%s\n", text);

                    // Note that no state change is needed.

                } else {


                }

            } else if (event == ......) {

                // TODO: Process other events.


            }





        }

        // Now we finished processing the pending event. Just go back to the
        // beginning of the loop and waiting for another event. 
        // Note that you can set a timeout for the select() function 
        // to allow it to return regularly and check timeout related events.

    } // This is the end of the while loop

} // This is the end of main()
