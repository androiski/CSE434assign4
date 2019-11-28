/*
Made by Andrew Murza for CSE434 Networks with Duo Lu
#1212940532
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

struct header {

    uint8_t magic1;
    uint8_t magic2;
    uint8_t opcode;
    uint8_t payload_len;

    uint32_t token;
    uint32_t msg_id;
};

const int h_size = sizeof(struct header);

#define MAGIC1  'A'
#define MAGIC2  'M'

// These are the constants indicating the states.
#define STATE_OFFLINE           0
#define STATE_LOGIN_SENT        1
#define STATE_ONLINE            2
#define STATE_LOGOUT_SENT       3
// Now you can define other states in a similar fashion.

// These are the constants indicating the events.
// All events starting with EVENT_USER_ are generated by a human user.
#define EVENT_USER_LOGIN                0
#define EVENT_USER_POST                 1
#define EVENT_USER_LOGOUT               2
#define EVENT_USER_RETRIEVE             3
#define EVENT_USER_SUB                  4
#define EVENT_USER_UNSUB                5
#define EVENT_USER_RESET                6
#define EVENT_FORWARD_ACK               7
// Now you can define other events from the user.

#define EVENT_USER_INVALID              79

// All events starting with EVENT_NET_ are generated by receiving a msg
// from the network. We deliberately use a larger numbers to help debug.
#define EVENT_NET_LOGIN_SUCCESSFUL      80
#define EVENT_NET_POST_ACK              81
#define EVENT_NET_LOGOUT_SUCCESSFUL     82
#define EVENT_NET_RET_ACK               83
#define EVENT_NET_RET_ACK_END           84
#define EVENT_NET_FORWARD               85
#define EVENT_NET_FAILED                86
#define EVENT_NET_SUB_SUCCESSFUL        87
#define EVENT_NET_UNSUB_SUCCESSFUL      88
// Now you can define other events from the network.

#define EVENT_NET_INVALID               255

// These are the constants indicating the opcodes.
#define OPCODE_RESET                    0x00
//client >> server opcodes
#define OPCODE_LOGIN                    0x10
#define OPCODE_SUBSCRIBE                0x20
#define OPCODE_UNSUBSCRIBE              0x21
#define OPCODE_POST                     0x30
#define OPCODE_FORWARD_ACK              0x31
#define OPCODE_RETRIEVE                 0x40
#define OPCODE_LOGOUT                   0x1F
//server >> client opcodes
#define OPCODE_MUST_LOGIN_FIRST_ERROR   0xF0
#define OPCODE_SUCCESSFUL_LOGIN_ACK     0x80 
#define OPCODE_FAILED_LOGIN_ACK         0x81
#define OPCODE_SUCCESSFUL_SUB_ACK       0x90
#define OPCODE_FAILED_SUB_ACK           0x91
#define OPCODE_SUCCESSFUL_UNSUB_ACK     0xA0
#define OPCODE_FAILED_UNSUB_ACK         0xA1
#define OPCODE_POST_ACK                 0xB0
#define OPCODE_FORWARD                  0xB1
#define OPCODE_RETRIEVE_ACK             0xC0
#define OPCODE_END_RETRIEVE_ACK         0xC1
#define OPCODE_LOGOUT_ACK               0x8F


// Now you can define other opcodes in a similar fashion.

int parse_the_event_from_the_input_string(char input_command[1024]){

    char loginhash[6] = {'l', 'o', 'g', 'i', 'n', '#'};
    char logouthash[7] = {'l', 'o', 'g', 'o', 'u', 't', '#'};
    char posthash[5] = {'p', 'o', 's', 't', '#'};
    char subscribehash[10] = {'s', 'u', 'b', 's', 'c', 'r', 'i', 'b', 'e', '#'};
    char unsubhash[12] = {'u', 'n', 's', 'u', 'b', 's', 'c', 'r', 'i', 'b', 'e', '#'};
    char retrievehash[9] = {'r', 'e', 't', 'r', 'i', 'e', 'v', 'e', '#'};
    char resethash[6] ={'r', 'e', 's', 'e', 't', '#'};

        if(strncmp(input_command, loginhash, 6) == 0){
            //printf("loging in ...\n");
            return EVENT_USER_LOGIN;
        }
        else if(strncmp(input_command, logouthash, 7) == 0){
           // printf("loging out ...\n");
            return EVENT_USER_LOGOUT;
        }
        else if(strncmp(input_command, posthash, 5) == 0){
           // printf("posting ...\n");
            return EVENT_USER_POST;
        }
        else if(strncmp(input_command, subscribehash, 10) == 0){
            //printf("subscribing ...\n");
            return EVENT_USER_SUB;
        }
        else if(strncmp(input_command, unsubhash, 12) == 0){
            //printf("unsubscribing ...\n");
            return EVENT_USER_UNSUB;
        }
        else if(strncmp(input_command, retrievehash, 9) == 0){
            //printf("retrieving ...\n");
            return EVENT_USER_RETRIEVE;
        }
        else if(strncmp(input_command, resethash, 6) == 0){
            //printf("reset ...\n");
            return EVENT_USER_RESET;
        }
        else{
            return EVENT_USER_RESET;
        }

}

int parse_the_event_from_the_received_message(uint8_t opcode){
    if(opcode == OPCODE_SUCCESSFUL_LOGIN_ACK){
        //printf("login ack\n");
        return EVENT_NET_LOGIN_SUCCESSFUL;
    }
    else if(opcode == OPCODE_LOGOUT_ACK){
       //printf("logout ack\n");
        return EVENT_NET_LOGOUT_SUCCESSFUL;
    }
    else if(opcode == OPCODE_FAILED_LOGIN_ACK){
        //printf("failed login ack\n");
        return EVENT_NET_FAILED;
    }
    else if(opcode == OPCODE_SUCCESSFUL_SUB_ACK){
       // printf("sub ack\n");
        return EVENT_NET_SUB_SUCCESSFUL;
    }
    else if(opcode == OPCODE_FAILED_SUB_ACK){
        //printf("failed sub ack\n");
        return EVENT_NET_FAILED;
    }
    else if(opcode == OPCODE_SUCCESSFUL_UNSUB_ACK){
      //  printf("unsub ack\n");
        return EVENT_NET_UNSUB_SUCCESSFUL;
    }
    else if(opcode == OPCODE_FAILED_UNSUB_ACK){
       // printf("failed unsub ack\n");
        return EVENT_NET_FAILED;
    }
    else if(opcode == OPCODE_POST_ACK){
       // printf("post ack\n");
        return EVENT_NET_POST_ACK;
    }
    else if(opcode == OPCODE_FORWARD){
        //printf("forward\n");
        return EVENT_NET_FORWARD;
    }
    else if(opcode == OPCODE_RETRIEVE_ACK){
       // printf("ret ack\n");
        return EVENT_NET_RET_ACK;
    }
    else if(opcode == OPCODE_END_RETRIEVE_ACK){
        //printf("ret ack end\n");
        return EVENT_NET_RET_ACK_END;
    }
    else{
       // printf("wtf\n");
        return -1;
    }
}


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

    //by default the magic letters will be my own, until defined by the EVENT_LOGIN_SUCCESSFUL state


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
    my_addr.sin_port = htons(rand() % (30000 + 1 - 5000) + 5000);//semi-random port number b/w 30000-9999 since 0-1023 are typically reserved


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
        memset(&recv_buffer, 0, sizeof(recv_buffer));
        memset(&send_buffer, 0, sizeof(send_buffer));
    
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
            event = parse_the_event_from_the_input_string(user_input);

            // You can also add a line to print the "event" for debugging.

            if (event == EVENT_USER_LOGIN) {
                    if (state != STATE_ONLINE) {

                        // CAUTION: we do not need to parse the user ID and
                        // and password string, assuming they are always in the
                        // correct format. The server will parse it anyway.

                        char *id_password = user_input + 6; // skip the "login#"
                        int m = strlen(id_password);

                        ph_send->magic1 = MAGIC1;
                        ph_send->magic2 = MAGIC2;
                        ph_send->opcode = OPCODE_LOGIN;
                        ph_send->payload_len = m;
                        ph_send->token = 0;
                        ph_send->msg_id = 0;

                        memcpy(send_buffer + h_size, id_password, m);

                        sendto(sockfd, send_buffer, h_size + m, 0, 
                            (struct sockaddr *) &serv_addr, sizeof(serv_addr));

                        // Once the corresponding action finishes, transit to
                        // the login_sent state
                        state = STATE_LOGIN_SENT;

                    } 
                    else {

                        // TODO: handle errors if the event happens in a state
                        // that is not expected. Basically just print an error
                        // message and doing nothing. Note that if a user types
                        // something invalid, it does not need to trigger a 
                        // session reset
                        printf("ERROR. You are already online!\n");

                    }

            } 
            else if (event == EVENT_USER_POST) {

                // Note that this is similar to the login msg.
                // Actually, these messages are carefully designed to 
                // somewhat minimize the processing on the client side.
                // If you look at the "subscribe", "unsubscribe", "post"
                // and "retrieve", they are all similar, i.e., just fill
                // the header and copy the user input after the "#" as
                // the payload of the message, then just send the msg.
                if (state == STATE_OFFLINE) {
                    printf("ERROR. You are already logged out!\n");
                }
                else {
                char *posted_msg = user_input + 5; // skip the "post#"
            
                char *delimiter = strchr(posted_msg, '\n');
                *delimiter = 0;
                int m = strlen(posted_msg);

                //printf("posted this: %s\n", posted_msg);

                ph_send->magic1 = MAGIC1;
                ph_send->magic2 = MAGIC2;
                ph_send->opcode = OPCODE_POST;
                ph_send->payload_len = m;
                ph_send->token = token;
                ph_send->msg_id = 0;

                memcpy(send_buffer + h_size, posted_msg, m);

                sendto(sockfd, send_buffer, h_size + m, 0, 
                    (struct sockaddr *) &serv_addr, sizeof(serv_addr));
                }


            } 
            else if (event == EVENT_USER_LOGOUT) {

                if (state == STATE_OFFLINE) {
                    printf("ERROR. You are already logged out!\n");
                }
                else {
                    int m = strlen(user_input);

                    ph_send->magic1 = MAGIC1;
                    ph_send->magic2 = MAGIC2;
                    ph_send->opcode = OPCODE_LOGOUT;
                    ph_send->payload_len = m;
                    ph_send->token = token;
                    ph_send->msg_id = 0;

                    memcpy(send_buffer + h_size, user_input, m);

                    sendto(sockfd, send_buffer, h_size + m, 0, 
                        (struct sockaddr *) &serv_addr, sizeof(serv_addr));

                    // Once the corresponding action finishes, transit to
                    // the logout_sent state
                    state = STATE_LOGOUT_SENT;
                }

            } 
            else if (event == EVENT_USER_RETRIEVE) {

                if (state == STATE_OFFLINE) {
                    printf("ERROR. Please log in!\n");
                }
                else {
                    char *ret_amt = user_input + 9;//save the txt after # 
                    char *delimiter = strchr(ret_amt, '\n');
                    *delimiter = 0;
                    int m = strlen(user_input);
                    printf("retrieving the %s latest posts\n", ret_amt);

                    ph_send->magic1 = MAGIC1;
                    ph_send->magic2 = MAGIC2;
                    ph_send->opcode = OPCODE_RETRIEVE;
                    ph_send->payload_len = m;
                    ph_send->token = token;
                    ph_send->msg_id = 0;

                    memcpy(send_buffer + h_size, ret_amt, m);

                    sendto(sockfd, send_buffer, h_size + m, 0, 
                        (struct sockaddr *) &serv_addr, sizeof(serv_addr));

                }

            } 
            else if (event == EVENT_USER_SUB) {

                if (state == STATE_OFFLINE) {
                    printf("ERROR. Please log in!\n");
                }
                else {
                    char *sub_to = user_input + 10;//save the user to sub to after #
                    char *delimiter = strchr(sub_to, '\n');
                    *delimiter = 0;
                    printf("subscribing to %s\n", sub_to);
                    int m = strlen(user_input);

                    ph_send->magic1 = MAGIC1;
                    ph_send->magic2 = MAGIC2;
                    ph_send->opcode = OPCODE_SUBSCRIBE;
                    ph_send->payload_len = m;
                    ph_send->token = token;
                    ph_send->msg_id = 0;

                    memcpy(send_buffer + h_size, sub_to, m);

                    sendto(sockfd, send_buffer, h_size + m, 0, 
                        (struct sockaddr *) &serv_addr, sizeof(serv_addr));
                        
                }

            } 
            else if (event == EVENT_USER_UNSUB) {

                if (state == STATE_OFFLINE) {
                    printf("ERROR. Please log in!\n");
                }
                else {
                    char *unsub_to = user_input + 12;//save the user to unsub to after #
                    char *delimiter = strchr(unsub_to, '\n');
                    *delimiter = 0;
                    printf("subscribing to %s\n", unsub_to);
                    int m = strlen(user_input);

                    ph_send->magic1 = MAGIC1;
                    ph_send->magic2 = MAGIC2;
                    ph_send->opcode = OPCODE_UNSUBSCRIBE;
                    ph_send->payload_len = m;
                    ph_send->token = token;
                    ph_send->msg_id = 0;

                    memcpy(send_buffer + h_size, unsub_to, m);

                    sendto(sockfd, send_buffer, h_size + m, 0, 
                        (struct sockaddr *) &serv_addr, sizeof(serv_addr));
                        
                }

            } 
            else if (event == EVENT_USER_RESET) {

                // TODO: You may add another command like "reset#" so as to
                // facilitate testing. In this case, a user just need to 
                // type this line to generate a reset message.
                
                ph_send->magic1 = MAGIC1;
                ph_send->magic2 = MAGIC2;
                ph_send->opcode = OPCODE_RESET;
                ph_send->payload_len = 0;
                ph_send->token = token;
                ph_send->msg_id = 0;

                    sendto(sockfd, send_buffer, h_size, 0, 
                        (struct sockaddr *) &serv_addr, sizeof(serv_addr));
                // You can add more command as you like to help debugging.
                // For example, I can add a command "state#" to instruct the
                // client program to print the current state without chang
                // -ing anything.
                printf("BAD/RESET COMMAND: CLIENT HAS BEEN RESET\n");
                state = STATE_OFFLINE;
                token = 0;


            } 

        }
        if (FD_ISSET(sockfd, &read_set)) {

            // Now we know there is an event from the network
            // TODO: Figure out which event and process it according to the
            // current state

            ret = recv(sockfd, recv_buffer, sizeof(recv_buffer), 0);
            event = parse_the_event_from_the_received_message(ph_recv->opcode);

            if (event == EVENT_NET_LOGIN_SUCCESSFUL) {
                if (state == STATE_LOGIN_SENT) {

                token = ph_recv->token;
                printf("Your token is %d\n", token);

                printf("LOGIN SUCESSFUL! YOU ARE NOW ONLINE\n");
                state = STATE_ONLINE;

                } 
                else {
                    printf("uhhh\n");

                    // A spurious msg is received. Just reset the session.
                    // You can define a function "send_reset()" for 
                    // convenience because it might be used in many places.
                    //send_reset(sockfd, send_buffer);

                    state = STATE_OFFLINE;
                }
            } 
            else if (event == EVENT_NET_FAILED) {
                if (state == STATE_LOGIN_SENT) {

                    printf("LOGIN FAILED!\n");
                    state = STATE_OFFLINE;

                } 
                else {

                   // send_reset(sockfd, send_buffer);

                    state = STATE_OFFLINE;
                }

                if (ph_recv->opcode == OPCODE_FAILED_SUB_ACK){
                    printf("SUBSCRIPTION FAILED!\n");
                }
                if (ph_recv->opcode == OPCODE_FAILED_UNSUB_ACK){
                    printf("UNSUBSCRIPTION FAILED!\n");
                }


            } 
            else if (event == EVENT_NET_FORWARD) {
                if (state == STATE_ONLINE) {

                    // Just extract the payload and print the text.
                    char *text = recv_buffer + h_size;

                    printf("%s\n", text);

                    // Note that no state change is needed.

                } 
                

            } 
            else if (event == EVENT_NET_LOGOUT_SUCCESSFUL) {

                if (state == STATE_LOGOUT_SENT) {

                printf("LOGOUT SUCESSFUL!\n");
                state = STATE_OFFLINE;

                } 
                else {

                    // A spurious msg is received. Just reset the session.
                    // You can define a function "send_reset()" for 
                    // convenience because it might be used in many places.
                    //send_reset(sockfd, send_buffer);

                    state = STATE_OFFLINE;
                }


            }
            else if (event == EVENT_NET_POST_ACK) {

                printf("POST SUCESSFUL!\n");

            }
            else if (event == EVENT_NET_RET_ACK) {

                char *text = recv_buffer + h_size;

                printf("%s\n", text);


            }
            else if (event == EVENT_NET_RET_ACK_END) {

                printf("DONE RETRIEVING!\n");


            }
            else if (event == EVENT_NET_SUB_SUCCESSFUL) {

                char *text = recv_buffer + h_size;

                printf("SUCESSFULLY SUBBED!\n");


            }
            else if (event == EVENT_NET_UNSUB_SUCCESSFUL) {

                char *text = recv_buffer + h_size;

                printf("SUCESSFULLY UNSUBBED!\n");


            }

        }

        // Now we finished processing the pending event. Just go back to the
        // beginning of the loop and waiting for another event. 
        // Note that you can set a timeout for the select() function 
        // to allow it to return regularly and check timeout related events.

    } // This is the end of the while loop

} // This is the end of main()

