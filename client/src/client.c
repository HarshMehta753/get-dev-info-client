/**********************************************************************************************************************************************
 * @file     client_sock.c
 * @brief    Client program for chat system which receives as well send msgs from/to server 
 * @author   Harsh Mehta
 * @version  0.0.1
 * @date     2026.08.27
 **********************************************************************************************************************************************/

#include "define.h" 

client_sock_config_t client_sock_config = {0}; /**< Client socket configuration structure. */
/* TODO: Optimise the code and use the below as requst type. I think we can remove from glboal and direct use on client_process() function. */
client_recv_req_type_t client_recv_req_type = CLEINT_RECV_REQ_NONE;


int client_init(void)
{

    /* TODO: Update the code with init functionality as mentioned below. */	

    /* TODO: 
     * -> In client init, we need to discover the server IP and server port using UDP brodcast method. 
     * -> Try to findout the server for 10 times at every 10 seconds. If after that still, cannot get the response, the init should fail, 
     *    else continue working.  
     * -> After succesfully the connection, server sends the unique ID, which will needs when the client sends the response. 
     * -> After connection, client send the request, which we need to parse and follow.  
     */	
    struct sockaddr_in client;
    int my_id; 
    pthread_t send_thread, recv_thread;
    client_sock_config.client_disconnected_f = false;
    
    /* Error check */
    if ( argc != 3)
    {
        printf ("Input @cmdline : ./client port_number server_ip_address\n");
        return -1;
    }
    
    /* Socket creation */
    client_sock_config.socket = socket(AF_INET, SOCK_STREAM, 0);
    if ( client_sock_config.socket == -1 )
    {
        printf ("Socket creation failed!\n");
        return -1;
    }
    
    /* TODO: Add discovery UDP logic and base on the IP and port connect to server using TCP. */
    /* Adding port and address info */
    client.sin_family = AF_INET;
    client.sin_port = htons(atoi(argv[1]));
    inet_aton (argv[2], &client.sin_addr); 
 
    /* Building connection to the server */
    if (connect (client_sock_config.socket, (struct sockaddr *)&client, sizeof(client)) == -1)
    {
        perror ("Connect Error");
        return -1;
    }
    
    /* check for data reception from server */
    if ( recv (client_sock_config.socket, &my_id, sizeof(my_id), 0) <= 0 )
    {
        printf ("Disconnected from server!\n");
        return -1;
    }

    printf (">>>>>>>>>>>>>> Connected to the server! <<<<<<<<<<<<<<\n");
    printf ("My ID: %d\n", my_id);

    printf ("\nEnter:\nlist --> to see the available clients\nexit --> to exit from program\n<client_id>:<Msg> --> to send msgs to particular client\n\n");
    
    return 0;
}

int client_process(void)
{
	/* TODO: Add other process logic, like based on the receive request perform the task. */

	/* TODO: Change base on the client_recv_req_type_t and store globally. */
    char buffer[CLIENT_SOCK_RCV_SND_BUFF_SIZE] = {0}
/* TODO: Optimise the code and use the below as requst type. I think we can remove from glboal and direct use on client_process() function. */
client_recv_req_type_t client_recv_req_type = CLEINT_RECV_REQ_NONE;
    
    /* receives the data from server continously*/
    while (client_sock_config.client_disconnected_f == false)
    {
        memset ( buffer, 0 , strlen(buffer) );
        if ( recv (client_sock_config.socket, buffer, CLIENT_SOCK_RCV_SND_BUFF_SIZE - 1, 0) <= 0)
        {
            printf ("Server Down!\n");
            client_sock_config.client_disconnected_f = true;
            break;
        }
        buffer[strlen(buffer)] = '\0';
        printf("\n%s\n", buffer);
    }
    /* TODO: After receiving the request, you need to parse that to cli_parse.c file, which will run the command and get the output, 
     * which we need to reshare with server. 
     */

    /* TODO; So based below discription you need to add enum of client_recv_req_type_t */
    /* TODO: As of now only 5 commands applicable 
     * 1. hostname and IP address 
     * 2. create one file with prefix test.txt file and inside that add "hello world 1 ... 2"  
     * 3. another is delete the the line from that file. If the line doesn't present, then its fine. 
     * 4. Delete the entire file. 
     * 5. Give the output of "ls ~/Downloads" command.  
     *
     * */

    /* TODO: For running the command you can use popen or system command to run the get the output. */
}
