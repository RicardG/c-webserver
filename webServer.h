/*
    Custom Web Server
    By: Ricard Grace
*/

//networking headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

//multi-threading
#include <pthread.h>

//timing
#include <unistd.h>

//Standand
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <sys/stat.h>

//Error handling
#include <errno.h>

#define STREQU 0
#define NOERR 0
#define ERROR -1
#define PORT "80"
#define BACKLOG 10
#define PACK_SIZE 2048
#define BUFF_SIZE 16384
#define TYPE_SIZE 10
#define PATH_SIZE PATH_MAX
#define HTMLVER_SIZE 3

//default locations
#define DEFAULT_PAGE "/index.html"
#define ERROR404_PAGE "/404error.html"
#define ERROR400_PAGE "/400error.html"
#define ERROR501_PAGE "/501error.html"
#define DEFAULT_DIR "/webServerData"

#define ENCODE_SIZE 3
#define TRUE 1
#define FALSE 0

//HTML Versions
#define HTTPVER_10 "HTTP/1.0"
#define HTTPVER_11 "HTTP/1.1"

//Response Codes
#define RESPONSE_200 "200 OK"
#define RESPONSE_400 "400 Bad Request"
#define RESPONSE_404 "404 Not Found"
#define RESPONSE_501 "501 Not Implemented"

//request types
#define REQUEST_GET     1000
#define REQUEST_HEAD    1001
#define REQUEST_POST    1002
#define REQUEST_PUT     1003
#define REQUEST_DELETE  1004
#define REQUEST_CONNECT 1005
#define REQUEST_OPTIONS 1006
#define REQUEST_TRACE   1007
#define REQUEST_INVALID 1999

typedef struct _requestInfo {
    int reqType;
    char fullAddress[PATH_SIZE*2+2];
    char fileName[PATH_SIZE+1];
    char* httpVer;
    char* responseCode;
    long long fileSize;
} ReqInfo;

void* ServePage (void* newConn);
void SendFile (char* address, int connID);
int RequestType (char* request, int bytesRecv);
char* FileAddress (char* request, int bytesRecv, char* result, char* fileName);
ReqInfo ProcessRequest (char* request, int bytesRecv);
int NextNonSpace (char* text, int length, int startPos);
int ConvertControlChar (char* text);
void SigPipeHandle (int i);
void CreateFullFileAddress (char* fullAddress, char* address);
char* Concat (char* str1, char* str2);
