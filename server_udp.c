#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <time.h>

struct header {

    char magic1;
    char magic2;
    char opcode;
    char payload_len;

    uint32_t token;
    uint32_t msg_id;
};

const int h_size = sizeof(struct header);

#define MAGIC1  'A'
#define MAGIC2  'M'
// These are the constants indicating the states.
// CAUTION: These states have nothing to do with the states on the client.
#define STATE_OFFLINE          0
#define STATE_ONLINE           1
#define STATE_MSG_FORWARD      2
// Now you can define other states in a similar fashion.

// These are the events
// CAUTION: These events have nothing to do with the states on the client.
#define EVENT_NET_LOGIN                 80
#define EVENT_NET_POST                  81
#define EVENT_NET_RETRIEVE              82
#define EVENT_NET_LOGOUT                83
#define EVENT_NET_SUB                   84
#define EVENT_NET_UNSUB                 85
#define EVENT_NET_FORWARD_ACK           86
// Now you can define other events from the network.

#define EVENT_NET_INVALID               255

// These are the constants indicating the opcodes.
// CAUTION: These opcodes must agree on both sides.
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
#define OPCODE_SUCCESSFULL_UNSUB_ACK    0xA0
#define OPCODE_FAILED_UNSUB_ACK         0xA1
#define OPCODE_POST_ACK                 0xB0
#define OPCODE_FORWARD                  0xB1
#define OPCODE_RETRIEVE_ACK             0xC0
#define OPCODE_END_RETRIEVE_ACK         0xC1
#define OPCODE_LOGOUT_ACK               0x8F

// Now you can define other opcodes in a similar fashion.

// This is a data structure that holds important information on a session.
struct session {

    char client_id[32]; // Assume the client ID is less than 32 characters.
    char client_pass[32];
    int subs[16];
    struct sockaddr_in client_addr; // IP address and port of the client
                                    // for receiving messages from the 
                                    // server.
    time_t last_time; // The last time when the server receives a message
                      // from this client.
    uint32_t token;        // The token of this session.
    int state;        // The state of this session, 0 is "OFFLINE", etc.

    // TODO: You may need to add more information such as the subscription
    // list, password, etc.
};

int parse_the_event(char opcode){
    if (opcode == OPCODE_LOGIN) {
        return EVENT_NET_LOGIN;
    }
    else if (opcode == OPCODE_LOGOUT) {
        return EVENT_NET_LOGOUT;
    }
    else if (opcode == OPCODE_POST) {
        return EVENT_NET_POST;
    }
    else if (opcode == OPCODE_SUBSCRIBE) {
        return EVENT_NET_SUB;
    }
    else if (opcode == OPCODE_UNSUBSCRIBE) {
        return EVENT_NET_UNSUB;
    }
    else if (opcode == OPCODE_FORWARD_ACK) {
        return EVENT_NET_FORWARD_ACK;
    }
    else if (opcode == OPCODE_RETRIEVE) {
        return EVENT_NET_RETRIEVE;
    }
}

// TODO: You may need to add more structures to hold global information
// such as all registered clients, the list of all posted messages, etc.
// Initially all sessions are in the OFFLINE state.

int main() {

    int ret;
    int sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    char send_buffer[1024];
    char recv_buffer[1024];
    int recv_len;
    socklen_t len;

    // You may need to use a std::map to hold all the sessions to find a 
    // session given a token. I just use an array just for demonstration.
    // Assume we are dealing with at most 16 clients, and this array of
    // the session structure is essentially our user database.
    struct session session_array[16];

    // Now you need to load all users' information and fill this array.
    // Optionally, you can just hardcode each user.
    struct session Noll;
    struct session Joe;
    struct session Mary;
    struct session Don;

    memset(&Noll, 0, sizeof(Noll));

    strcpy(Joe.client_id, "joeking");
    strcpy(Joe.client_pass, "lol");
    memset(&Joe.subs, 0, sizeof(Joe.subs));
    Joe.state = STATE_OFFLINE;

    strcpy(Mary.client_id, "mary2");
    strcpy(Mary.client_pass, "101");
    memset(&Mary.subs, 0, sizeof(Mary.subs));
    Mary.state = STATE_OFFLINE;

    strcpy(Don.client_id, "don42");
    strcpy(Don.client_pass, "password");
    memset(&Don.subs, 0, sizeof(Don.subs));
    Don.state = STATE_OFFLINE;

    session_array[0] = Noll;    
    session_array[1] = Joe;
    session_array[2] = Mary;
    session_array[3] = Don;

    // This current_session is a variable temporarily hold the session upon
    // an event.
    struct session *current_session;
    int token;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("socket() error: %s.\n", strerror(errno));
        return -1;
    }

    // The servaddr is the address and port number that the server will 
    // keep receiving from.
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(32000);

    bind(sockfd, 
         (struct sockaddr *) &serv_addr, 
         sizeof(serv_addr));

    // Same as that in the client code.
    struct header *ph_send = (struct header *)send_buffer;
    struct header *ph_recv = (struct header *)recv_buffer;

    while (1) {

        // Note that the program will still block on recvfrom()
        // You may call select() only on this socket file descriptor with
        // a timeout, or set a timeout using the socket options.

        len = sizeof(cli_addr);
        recv_len = recvfrom(sockfd, // socket file descriptor
                 recv_buffer,       // receive buffer
                 sizeof(recv_buffer),  // number of bytes to be received
                 0,
                 (struct sockaddr *) &cli_addr,  // client address
                 &len);             // length of client address structure

        if (recv_len <= 0) {
            printf("recvfrom() error: %s.\n", strerror(errno));
            return -1;
        }




        // Now we know there is an event from the network
        // TODO: Figure out which event and process it according to the
        // current state of the session referred.

        struct session *cs;
        int token = ph_recv->token;
        // This is the current session we are working with.
        for(int i = 1; i <= 16; i++){
            struct session *temp = &session_array[i];
            int temp_token = temp->token;
            if(temp_token == token){
                cs = temp;
            }
            else{
                cs = &session_array[0]; //where Noll the NULL session is located
            }
        }
        int event = parse_the_event(ph_recv->opcode);

        // Record the last time that this session is active.
        current_session->last_time = time(0);

        if (event == EVENT_NET_LOGIN) {

            // For a login event, the current_session should be NULL and
            // the token is 0.

            char *id_password = recv_buffer + h_size;

            char *delimiter = strchr(id_password, '&');
            char *password = delimiter + 1;
            *delimiter = 0; // Add a null terminator
            // Note that this null terminator can break the user ID
            // and the password without allocating other buffers.
            char *user_id = id_password;

            delimiter = strchr(password, '\n');
            *delimiter = 0; // Add a null terminator
            // Note that since we did not process it on the client side,
            // and since it is always typed by a user, there must be a
            // trailing new line. We just write a null terminator on this
            // place to terminate the password string.




            // The server need to reply a msg anyway, and this reply msg
            // contains only the header
            ph_send->payload_len = 0;
            ph_send->msg_id = 0;
            
            //check password
            int login_success = 0;

            for(int i = 1; i <= 16; i++){
                struct session *temp = &session_array[i];
                if(strcmp(password, temp->client_pass) == 0 && strcmp(user_id, temp->client_id) == 0){
                    login_success = 1;
                    cs = temp;
                }

            }
            if (login_success > 0) {

                // This means the login is successful.

                ph_send->opcode = OPCODE_SUCCESSFUL_LOGIN_ACK;
                ph_send->token = 1 + rand() %(1000);

                
                cs->state = STATE_ONLINE;
                cs->token = ph_send->token;
                cs->last_time = right_now();
                cs->client_addr = cli_addr;

            } else {

                ph_send->opcode = OPCODE_FAILED_LOGIN_ACK;
                ph_send->token = 0;

            }

            sendto(sockfd, send_buffer, h_size, 0, 
                (struct sockaddr *) &cli_addr, sizeof(cli_addr));



        } else if (event == EVENT_NET_POST) {
            // TODO: Check the statetoken of the client that sends this post msg,
            // i.e., check cs->statetoken.

            // Now we assume it is ONLINE, because I do not want to ident
            // the following code in another layer.

            for (int i = 1; i <= 16; i++) {
                
                if(cs->subs[i] == 1){
                    struct session *target = &session_array[i];
                    
                    char *text = recv_buffer + h_size;
                    char *payload = send_buffer + h_size;

                    // This formatting the "<client_a>some_text" in the payload
                    // of the forward msg, and hence, the client does not need
                    // to format it, i.e., the client can just print it out.
                    snprintf(payload, sizeof(send_buffer) - h_size, "<%s>%s",
                        cs->client_id, text);

                    int m = strlen(payload);

                    // "target" is the session structure of the target client.
                    target->state = STATE_MSG_FORWARD;

                    ph_send->magic1 = MAGIC1;
                    ph_send->magic2 = MAGIC2;
                    ph_send->opcode = OPCODE_FORWARD;
                    ph_send->payload_len = m;
                    ph_send->msg_id = 0; // Note that I didn't use msg_id here.

                    sendto(sockfd, send_buffer, h_size, 0, 
                        (struct sockaddr *) &target->client_addr, 
                        sizeof(target->client_addr));
                }
                

            }

            // TODO: send back the post ack to this publisher.

            // TODO: put the posted text line into a global list.


        } else if (event == ...) {

            // TODO: process other events

        }

        time_t current_time = time();

        // Now you may check the time of clients, i.e., scan all sessions. 
        // For each session, if the current time has passed 5 minutes plus 
        // the last time of the session, the session expires.
        // TODO: check session liveliness


    } // This is the end of the while loop

    return 0;
} // This is the end of main()
