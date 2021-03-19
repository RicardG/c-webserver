/*
    Custom Web Server
    By: Ricard Grace
*/

#include "webServer.h"

/***** Things to do *****
    * server to handle and accept incoming connections
    * create a new thread for each incoming request accepted
    * process the http request to determine what should be send back
        * process get requests
        * determine which file the client is requesting
    * create file I/O handler to find the requested file
        * load parts of the file into a buffer and send to the client in chunks
        * handle files within folders
        * handle non-existant files with a 404 error
        - stop unauthorised access of files
    * add error handling for all error prone requests
        * check that the connection to the client is still open before sending the next chunk
        * create function that returns an error string
    * create status request generator
    * create function that properly gets the HTTP request (read until \n\n) before processing
    * prevent the user from accessing files outside of the webServerData directory
    - prevent the server from opening and returning directory 'files'
    - create a redirection lookup table
*/

int main () {
    printf("==============================\n");
    printf("Booting Web Server\n");
    printf("Version: 1.4\n");
    printf("==============================\n");
    
    //setup multithreading
    pthread_t thread;

    //setup server
    int addrErr;
    struct addrinfo serverSettings; //contains server settings
    struct addrinfo *hostInfo;      //contains the host's network info
    memset(&serverSettings,0,sizeof(serverSettings));
    serverSettings.ai_family = AF_INET;     //network type (ipv4)
    serverSettings.ai_socktype = SOCK_STREAM;  //connection type (TCP streaming)
    serverSettings.ai_flags = AI_PASSIVE;      //IP address binding (auto)
    
    printf("Starting server setup...\n");
    addrErr = getaddrinfo(NULL,PORT,&serverSettings,&hostInfo);
    if (addrErr != NOERR) {
        //error handling
        fprintf(stderr,"** getaddrinfo error: %s **\n",gai_strerror(addrErr));
        exit(1);
    }
    
    //socket setup
    int listenSoc;
    printf("Socket setup...\n");
    listenSoc = socket(hostInfo->ai_family,hostInfo->ai_socktype,hostInfo->ai_protocol);
    if (listenSoc == ERROR) {
        //error handling
        fprintf(stderr,"** socket error **\n");
        exit(1);
    }

    //bind to port and socket
    int bindErr;
    printf("Binding to port...\n");
    bindErr = bind(listenSoc,hostInfo->ai_addr,hostInfo->ai_addrlen);
    if (bindErr == ERROR) {
        //error handling
        fprintf(stderr,"** binding error **\n");
        exit(1);
    }

    //listen on port
    int listenErr;
    printf("Listening to port...\n");
    listenErr = listen(listenSoc,BACKLOG);
    if (listenErr == ERROR) {
        fprintf(stderr,"** listen error **\n");
        exit(1);
    }

    printf("Setting up error handles...\n");
    signal(SIGPIPE,SigPipeHandle);

    printf("Server Setup Complete!\n");
    printf("Waiting for Clients\n");
    printf("==============================\n");
    
    struct sockaddr_storage connInfo;
    socklen_t connInfoSize = sizeof(connInfo);
    while (1) {
        //reset for new connection
        memset(&connInfo,0,connInfoSize);

        //wait for incoming connections
        int* newConn = malloc(sizeof(int));
        *newConn = accept(listenSoc,(struct sockaddr*)&connInfo,&connInfoSize);
        if (*newConn == ERROR) {
            //error checking
            fprintf(stderr,"** accept error **\n");
            exit(1);
        }
        //we have a valid connection, start processing request
        pthread_create(&thread,NULL,ServePage,(void*)newConn);
    }
    
    freeaddrinfo(hostInfo);
    return 0;
}

char* Concat (char* str1, char* str2) {
    int str1Len = strlen(str1);
    int str2Len = strlen(str2);
    char* newStr = malloc(sizeof(char) * (str1Len + str2Len + 1));
    int i;
    for (i = 0; i < str1Len; i++) {
        newStr[i] = str1[i];
    }
    int j;
    for (j = 0; j < str2Len; j++) {
        newStr[i+j] = str2[j];
    }
    newStr[i+j] = '\0';
    return newStr;
}

int ReadHTTPRequest(char* buffer, int connID) {
    //keep reading the request into the buffer until a double new line is encountered (it will be in the form /r/n/r/n or /n/n)
    //assume only one request will be made per tcp connection
    int endRequest = FALSE;
    int bytesRead = 0;
    int recvOut = 0;
    printf("- Getting Client Request...");
    while (!endRequest) {
        //read into buffer
        //keep track of how much we have read into the buffer, the position to start reading into and how much more we can read into the buffer
        recvOut = recv(connID, &buffer[bytesRead], BUFF_SIZE-bytesRead,0);
        if (recvOut == ERROR) {
            //some error
            fprintf(stderr,"** recv error ** %s\n",strerror(errno));
            return ERROR;
        } else if (recvOut == 0) {
            //the client has terminated the connection
            printf("Connection Terminated\n");
            return ERROR;
        }

        //if we get here then we recieved the data correctly
        //check if the data received contained a double return
        int i;
        for (i = bytesRead; i< bytesRead+recvOut; i++) {
            //this should loop through all the characters that were just placed into the buffer
            if (strstr(buffer, "\r\n\r\n")){// || strstr(buffer, "\n\n")) {
                printf("Found end\n");
                return bytesRead+recvOut;
            }
        }
        bytesRead += recvOut;
    }
    
    return ERROR;
}

void* ServePage (void* newConn) {
    //save the connection id on the stack and free the temp memory
    int connID = *(int*)newConn;
    free(newConn);
    printf("=== NEW CONNECTION ===\n");
    //the request needs to be read first
    char recvBuffer[BUFF_SIZE+1];
    memset(recvBuffer,0,BUFF_SIZE+1);
    //int bytesRecv = recv(connID,buffer,BUFF_SIZE,0);
    int bytesRecv;
    if ((bytesRecv = ReadHTTPRequest(recvBuffer,connID)) == ERROR) {
        //error reading the request
        printf("Error reading request\n");
        printf("TERMINATING\n");
        return NULL;
    }

    /*
    //Determine the kind of request (GET,POST,...)
    int reqType = RequestType(buffer, bytesRecv);
    */
    
    printf("- Serving webpage...\n");
    ReqInfo reqInfo = ProcessRequest(recvBuffer, bytesRecv);
    
    //send the HTTP response header
    char sendBuffer[BUFF_SIZE+1];
    memset(sendBuffer,0,BUFF_SIZE+1);
    snprintf(sendBuffer,BUFF_SIZE+1,"%s %s\nContent-Length: %lld\n\n",reqInfo.httpVer,reqInfo.responseCode,reqInfo.fileSize);
    send(connID,sendBuffer,strlen(sendBuffer),0);

    //now send the attatched file
    if (reqInfo.fileSize > 0 && reqInfo.reqType != REQUEST_HEAD) {
        SendFile(reqInfo.fullAddress,connID);
    }

    /*
    //if GET request
    if (reqInfo.reqType == REQUEST_GET) {
        //printf("*%s*\n",buffer);
        //process the http request and find the requested file
        //insert function here (returns a file address string)
        char buffer[BUFF_SIZE+1];
        memset(buffer,0,BUFF_SIZE+1);
        snprintf(buffer,BUFF_SIZE+1,"%s %s\nContent-Length: %lld\n\n",reqInfo.httpVer,reqInfo.responseCode,reqInfo.fileSize);
        //char* head1 = "HTTP/1.0 200 OK\n";
        //char* head2 = "Content-Length: "
        send(connID,buffer,strlen(buffer),0);
        //send contents of file at requested location
        //insert function here
        //SendFile("index.html",connID);
        SendFile(reqInfo.fullAddress,connID);
    } else if (reqInfo.reqType == REQUEST_INVALID) {
        char* msg = "HTTP/1.0 400 Bad Request\n\n";
        send(connID,msg,strlen(msg),0);
        SendFile(reqInfo.fullAddress,connID);
    } else {
        char* msg = "HTTP/1.0 501 Not Implemented\n\n";
        send(connID,msg,strlen(msg),0);
    }
    */

    //finished sending info, kill connection
    close(connID);
    printf("Done!\n");
    
    return NULL;
}


void SigPipeHandle (int i) {
    printf("SIGPIPE - throwing error\n");
    return;
}

void SendFile (char* address, int connID) {
    printf("Sending file: (%s)\n",address);

    FILE* file = fopen(address,"r");
    if (file == NULL) {
        fprintf(stderr,"** FILE DOES NOT EXIST **\n");
        return;
    }
    char buffer[PACK_SIZE+1];
    memset(buffer,0,PACK_SIZE+1);

    long long totalRead = 0;
    int elemRead = 0;
    int bytesSent = 0;
    int totalSent = 0;
    //put read and send into a loop to send all data in a file until done
    while ((elemRead = (int)fread(buffer,1,PACK_SIZE,file)) > 0) {
        totalRead+=elemRead;
        totalSent = 0;
        while (totalSent < elemRead) {
            bytesSent = send(connID,buffer,elemRead-totalSent,0);
            if (bytesSent == ERROR) {//change later
                //error handling
                fprintf(stderr,"** send error ** %s\n",strerror(errno));

                return;
            }
            totalSent += bytesSent;
        }
    }
}

int RequestType (char* request, int bytesRecv) {
    //get the first word in the request and check if it is any of the allowed words.
    //read the request until the first space is encountered or we reach the buffer limit
    char buffer[TYPE_SIZE+1];
    memset(buffer,0,TYPE_SIZE+1);
    int i;
    for (i = 0; i < TYPE_SIZE && i < BUFF_SIZE && i < bytesRecv;i++) {
        if (request[i] == ' ' || request[i] == '\0' || request[i] == '\r') break;
        buffer[i] = request[i];
    }
    //the first word is now in the buffer
    printf("REQUEST: '%s'\n",buffer);
    //convert the text into a number representing the request type
    if (strcmp(buffer,"GET") == STREQU) return REQUEST_GET;
    if (strcmp(buffer,"HEAD") == STREQU) return REQUEST_HEAD;
    if (strcmp(buffer,"POST") == STREQU) return REQUEST_POST;
    if (strcmp(buffer,"PUT") == STREQU) return REQUEST_PUT;
    if (strcmp(buffer,"DELETE") == STREQU) return REQUEST_DELETE;
    if (strcmp(buffer,"CONNECT") == STREQU) return REQUEST_CONNECT;
    if (strcmp(buffer,"OPTIONS") == STREQU) return REQUEST_OPTIONS;
    if (strcmp(buffer,"TRACE") == STREQU) return REQUEST_TRACE;
    
    //the client supplied a malformed request
    return REQUEST_INVALID;
}

//assumes that fullAddress contains at least PATH_SIZE*2+2 space
void CreateFullFileAddress (char* fullAddress, char* fileName) {
    char cwd[PATH_SIZE+2];
    memset(cwd,0,PATH_SIZE+2);
    memset(fullAddress,0,PATH_SIZE*2+2);
    getcwd(cwd,PATH_SIZE+1);
    //append the requested file to the end of cwd
    
    snprintf(fullAddress,BUFF_SIZE*2+2, "%s%s%s", cwd, DEFAULT_DIR, fileName);
    /*
    int i,l;
    for (i = 0; fullAddress[i] != '\0' && i < PATH_SIZE; i++) {} 
    
    for (l = 0; address[l] != '\0' && l < PATH_SIZE; l++) {
        fullAddress[i+l] = address[l];
    }
    */
    //printf("Result: (%s)\n",fullAddress);
    
    return;
}

//returns a pointer to the response code generated
char* FileAddress (char* request, int bytesRecv, char* result, char* fileName) {
    //expect text and then a space before the address
    //the address does not contain any spaces in its raw form
    //'%' is a command character which can be followed by numbers to represent a character
    //stop reading at the new space or when we reach the end of the request
    int isAddress = FALSE;
    char* file = NULL;
    char* response = NULL;
    char fullAddress[PATH_SIZE*2+2];
    char buffer[PATH_SIZE+1];
    memset(buffer,0,PATH_SIZE+1);
    memset(fullAddress,0,PATH_SIZE*2+2);
    //printf("Full: (%p|%p)\n",fullAddress,buffer);

    int i;
    for(i = 0; i < bytesRecv && i < TYPE_SIZE; i++) {
        if (request[i] == ' ') {
            //we have found the space, look for the next non space character
            i = NextNonSpace(request, bytesRecv, i);
            if (request[i] != ' ' && bytesRecv >= i+2) isAddress = TRUE;
            break;
        }
    }

    if (isAddress == TRUE) {
        //read the address (up until the next space)
        char buffer[PATH_SIZE+1];
        memset(buffer,0,PATH_SIZE+1);
        int j;
        for (j = 0; j < PATH_SIZE && j+i < bytesRecv; j++) {
            if (request[i+j] != ' ' && request[i+j] != '\n' && request[i+j] != '\r') {
                if (request[i+j] == '%') {
                    //control character
                    char control[ENCODE_SIZE+1];
                    memset(control,0,ENCODE_SIZE+1);
                    
                    //add data to array
                    int p;
                    for (p = 0; p < ENCODE_SIZE && i+j+p < bytesRecv; p++) {
                        control[p] = request[i+j+p];
                    }
                    int r = ConvertControlChar(control);
                    if (r == ERROR) {
                        //this is not a control character, do nothing
                        buffer[j] = request[i+j];
                    } else if (r > 0) {
                        //this is an invalid control sequence, skip ahead r many positions
                        i+=r;
                        j--;
                    } else if (r == 0) {
                        //the conversion was a success
                        buffer[j] = control[0];
                        i+=ENCODE_SIZE-1;
                    }
                    //check for directory up (/../)
                } else if (i+j+3 < bytesRecv && (request[i+j] == '/' && request[i+j+1] == '.' && request[i+j+2] == '.' && request[i+j+3] == '/')) {
                    //this is a directory up command, ignore it (remove '../')
                    //printf("Up Dir Command Detected, Ignoring it\n");
                    //buffer[j] = request[i+j];
                    i+=3;
                    j--;
                } else {
                    buffer[j] = request[i+j];
                }
            } else {
                break;
            }        
        }
        file = buffer;
        printf("Requested Address: (%s) | ",file);
        if (file[0] == '/' && strlen(file) == 1) {
            file = DEFAULT_PAGE;
            response = RESPONSE_200;
        }
    } else {
        //no address was supplied
        printf("No address\n");
        file = ERROR400_PAGE;
        response = RESPONSE_400;
    }
    //append the current working directory to the start of the requested file
    CreateFullFileAddress (fullAddress, file);

    //check that the address in result actually exists
    FILE* checkF = fopen(fullAddress,"r");
    if (checkF != NULL) {
        //the file opened, so close the file
        fclose(checkF);
        response = RESPONSE_200;
    } else {
        //the file does not exist, return the 404error file
        file = ERROR404_PAGE;
        CreateFullFileAddress(fullAddress, file);
        response = RESPONSE_404;
    }

    //copy the full file address into the result string and return the response code
    //copy address into result string (len PATH_SIZE*2+2)
    int k;
    for (k = 0 ;k < PATH_SIZE*2+1 && fullAddress[k] != '\0'; k++) {
        result[k] = fullAddress[k];
    }

    //copy the file retrieved into file name
    for (k = 0; k < PATH_SIZE && file[k] != '\0'; k++) {
        fileName[k] = file[k];
    }
    printf("File To Serve: (%s)\n", file);

    return response;
}

int NextNonSpace (char* text, int length, int startPos) {
    //move along the char array until a non space is encountered or we reach the maximum length
    int i;
    for (i = startPos; i < length && text[i] == ' '; i++) {}
    //printf("NS: (%c) S=%d I=%d L=%d\n",text[i],startPos,i,length);
    return i;
}

//give function string with % sign (eg:%20) string must be length 3+NULL
//if successful, return 0, char will be in the first array slot
//if ERR return ERROR
//if char not hexadecimal return location of char in array
int ConvertControlChar (char* text) {
    //check that the control char actually exists at the first location
    if (text[0] != '%') return ERROR;
    
    int i;
    //check the other two characters are actually hexadecimal
    for (i = 0; i < ENCODE_SIZE-1; i++) {
        //check the character is actually hexadecimal
        if ((text[i+1] >= '0' && text[i+1] <='9') || (text[i+1] >= 'a' && text[i+1] <= 'f') || (text[i+1] >= 'A' && text[i+1] <= 'F')) {
            //then this is valid hexadecimal
            //printf("char: (%c) %d\n",text[i+1], text[i+1]);
        } else {
            //printf("Error in Loc %d\n", i);
            return i+1;
        }
    }

    text[0] = (char)strtol(text+1,NULL,16);
    //printf("char: (%c) %d\n",text[0], text[0]);
    return 0;

}

ReqInfo ProcessRequest (char* request, int bytesRecv) {
    //create and setup data structure
    ReqInfo reqInfo;
    memset(reqInfo.fullAddress,0, PATH_SIZE*2+2);
    memset(reqInfo.fileName,0,PATH_SIZE+1);

    //Determine the nature of the request (GET,...)
    reqInfo.reqType = RequestType(request, bytesRecv);

    //Determine the address in the request
    if (reqInfo.reqType == REQUEST_GET || reqInfo.reqType == REQUEST_HEAD) {
        reqInfo.responseCode = FileAddress(request, bytesRecv, reqInfo.fullAddress, reqInfo.fileName);
    } else if (reqInfo.reqType == REQUEST_INVALID) {
        reqInfo.responseCode = RESPONSE_400;
        CreateFullFileAddress(reqInfo.fullAddress,ERROR400_PAGE);
        snprintf(reqInfo.fileName,PATH_SIZE,ERROR400_PAGE);
    } else if (reqInfo.reqType == REQUEST_POST) {
        printf("%s\n\n",request);
    } else {
        //this is a request which is not implemented
        reqInfo.responseCode = RESPONSE_501;
        CreateFullFileAddress(reqInfo.fullAddress,ERROR501_PAGE);
        snprintf(reqInfo.fileName,PATH_SIZE,ERROR501_PAGE);
    }

    //set http version code
    reqInfo.httpVer = HTTPVER_10;

    //set file size of file that is being transmitted
    struct stat st;
    if (stat(reqInfo.fullAddress,&st) == ERROR) {
        printf("Could not get file stats\n");
        reqInfo.fileSize = 0;
    } else {
        //we need to get and use the file size
        //printf("File Size: %lldB\n",(long long)st.st_size);
        reqInfo.fileSize = (long long)st.st_size;
    }

    return reqInfo;
}
