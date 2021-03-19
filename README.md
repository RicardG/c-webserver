# C-Webserver Challenge Project
This project is a semi-functioning webserver written entirely in C. Its main purpose was as a challenge project to learn raw networking, threading, C and its nuances, file I/O, http protocol structure and html in a limited capacity. It supports multiple simultaneous clients, delivery of webpages and downloading of files to clients while handling common errors such as clients disconnecting mid-download. 

## How to use
Running this project requires only 3 steps
1. Build the webserver on your machine using the provided make file\
$`make`
2. Run the webserver. By default it will bind to the HTTP port (80). This can be changed in webserver.h, but you will need to rebuild the server each time you do it.\
$`./WebServer`
3. Connect to the webserver. If you ran the server on your current machine you can access it using your preferred web browser at http://localhost.

## I don't like the provided webpages and want to provide my own
The beauty of this project is webpages served to clients are not hardcoded in the webserver. Instead, they are dynamically loaded from files in the 'webServerData' folder. Anything you put in that folder can be accessed by a client if they know the URL (or have a link to it). Thus, customising the pages served by this webserver is as simple as copy-paste. Note: any page provided as index.html at the root of the 'webServerData' folder will act as the landing page for the website.