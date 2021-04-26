#include <PalmOS.h>
#include <Field.h>
#include <StdIOPalm.h>
#include "PalmGETRsc.h"
#include <Unix/sys_socket.h> // treat PalmOS sockets as UNIX sockets
Err errno; // required for sys_socket.h

// max string length a bit of temporary memory for stuff
// if the web page being retrieved is bigger than this, we're in for a Bad Time
#define BUF_LEN 4095
Char temp[BUF_LEN];

// the main text we're going to show
// text going into a form/field needs to be global in scope for some reason?
Char mytext[BUF_LEN];

// the web patch we're retrieving
#define TO_GET "/read.php?a=https%3A%2F%2Fguru3.net%2F&rut=6ed088717b37f78a330a2c3536d375d969e83775e40dc1f14bdbb0095915e977"
#define REMOTE_HOST "frogfind.com"
#define REMOTE_PORT 80

// function declarations
void fetchHTML();
int stripHTMLTags(char *sToClean,size_t size);

// entry point
UInt32 PilotMain( UInt16 cmd, void *cmdPBP, UInt16 launchFlags )
{
    // application is starting
    if (cmd == sysAppLaunchCmdNormalLaunch) {
        // for some reason all variables need to be at the start of a block
        EventType event;

        // for checking PalmOS version
        UInt32 romVersion;

        // formP for display
        FieldType *field, *urifield;
        FormPtr formP;
        ControlType *ctrl;
        MemHandle textH;

        // check PalmOS version here
        FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
        if (romVersion <= sysMakeROMVersion(3,5,0,0,0)) {
            WinDrawChars( "Too old", 7, 60, 20 );
            // I'm not really sure what will happen here, watch out!
            return;
        }

        // load formP resource
    	FrmGotoForm(PalmGETForm);
        formP = FrmInitForm(PalmGETForm);

        // main text field for display
        field = FrmGetObjectPtr(formP, FrmGetObjectIndex(formP, DataField));

        // text for main text field
        MemSet(mytext, BUF_LEN, 0);
        StrCopy(mytext, "Press GET");

        // apply text to main display text field
        textH = MemHandleNew(1+StrLen(mytext));
        StrCopy(MemHandleLock(textH), mytext);
        MemHandleUnlock(textH);
        FldSetTextHandle(field, textH);
        FldDrawField(field);

        // address bar
        urifield = FrmGetObjectPtr(formP, FrmGetObjectIndex(formP, URIField));
        textH = MemHandleNew(201);
        StrCopy(MemHandleLock(textH), "http://");
        MemHandleUnlock(textH);
        FldSetTextHandle(urifield, textH);
        FldDrawField(urifield);

		FrmSetActiveForm(formP); // activate the form for display
        FrmDrawForm(formP); // draw it (not required?)

        //  Main event loop:
        do {
            // get events, wait at most 1/10th of a second
            EvtGetEvent(&event, SysTicksPerSecond()/10);

            //  system gets first chance to handle the event
            if (!SysHandleEvent( &event )) {
			    FrmDispatchEvent(&event); // needed to get button presses to work
                switch (event.eType) {
                    case keyDownEvent:
                        // scroll up and down
                        if (event.data.keyDown.chr == pageUpChr) {
                            FldScrollField(field, 2, winUp);
                        } else if (event.data.keyDown.chr == pageDownChr) {
                            FldScrollField(field, 2, winDown);
                        }
                        break;

                    case ctlSelectEvent:

                        if (event.data.ctlSelect.controlID == GETButton) {
                            // do things when pressing the GET button
                            fetchHTML();

                            textH = FldGetTextHandle(field);
                            FldSetTextHandle(field, NULL);
                            if (textH) MemHandleFree(textH);
                            textH = MemHandleNew(1+StrLen(mytext));
                            StrCopy(MemHandleLock(textH), mytext);
                            MemHandleUnlock(textH);
                            FldSetTextHandle(field, textH);
                            FldDrawField(field);
                        }
                        break;
                        
                }
            }

        // return from PilotMain when an appStopEvent is received
        } while (event.eType != appStopEvent);

    }
    return;
}

// do the GET
void fetchHTML() {
    // variables for sockets
    int socket_desc;
    UInt16 badIF = 0;
    struct sockaddr_in server;
	struct hostent *hp;

    // vars for string manipulation
    Char *found;
    int i;

    // construct the HTTP request
    // sprintf is wrapped to StrPrintF by StdIOPalm.h
    MemSet(mytext, BUF_LEN, 0);
    sprintf(mytext, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", TO_GET, REMOTE_HOST);

    // check if the net library exists
    if (SysLibFind("Net.lib", &AppNetRefnum)) // returns 0 if not found?
    {
        StrCat(mytext, "Could not find NetLib\n");
    }
    else
    {   // check if we can open the network library (starts network connection?)
        if (NetLibOpen(AppNetRefnum, &badIF))
        {
            StrCat(mytext, "Could not open NetLib\n");
        }
        else
        {
            // create a socket and open it
            // https://www.binarytides.com/socket-programming-c-linux-tutorial/
           	socket_desc = socket(AF_INET, SOCK_STREAM , 0); 
            if (socket_desc == -1)
            {
                StrCat(mytext, "Could not create socket\n");
            }
            else
            {
                // convert hostname to IP address directly in socket
                hp = gethostbyname(REMOTE_HOST);
                bcopy(hp->h_addr, (char*)&server.sin_addr, hp->h_length);

                // IPv4 and specify port
                server.sin_family = AF_INET;
                server.sin_port = htons(REMOTE_PORT);

                // connect the socket
                if (connect(socket_desc, (struct sockaddr *)&server , sizeof(server)) < 0)
                {
                    StrCat(mytext, "Connect error\n");
                }
                else // connected
                {   // send HTTP request
                    if( send(socket_desc , mytext , StrLen(mytext) , 0) < 0)
                    {
                        StrCat(mytext, "Send failed\n");
                    }
                    else // data sent
                    {   // zero out buffer (needed to ensure string is null terminated)
                        // note the use of PalmOS functions, not c standard functions
                        MemSet(temp, BUF_LEN, 0);
                        // read as much data as is available and append it to mytext for display
                        // note this is not C standard read, but a wrapper for NetLibReceive
                        while (read(socket_desc, temp, BUF_LEN - 1) != 0){
                            StrCat(mytext, temp);
                            MemSet(temp, BUF_LEN, 0); // re-zero buffer
                        }
                        StrCat(mytext, "\n"); // a final new line

                        // strip out HTML tags to make display nicer on the screen
                        // if you're getting fatal exceptions, this may be the cause
                        stripHTMLTags(mytext, StrLen(mytext));
                    }
                }
                // make sure to close the socket after transfers are done or PalmOS may run out of them
                close(socket_desc);
            }

            // close the library when we go (or we may run out)
            NetLibClose(AppNetRefnum, false);
        }
    }

    // PalmOS can't display the carriage returns, replace them with new lines
    // maybe replace with TxtReplaceStr ?
    found = StrChr(mytext, '\r');
    while (found != NULL) {
        *found = '\n'; // ' ' 
        found = StrChr(mytext, '\r');
    }

    // remove double newlines and replace one with a space
    for (i = 0; i < StrLen(mytext)-1; i++) {
        if (mytext[i] == '\n' && mytext[i+1] == '\n') {
            mytext[i] = ' ';
        }
    }

    // append this to the end so we know when the data ends
    StrCat(mytext, "\nALL DONE2\n");
}

// function to strip tags, taken from
// https://stackoverflow.com/questions/9444200/c-strip-html-between
// with standard c string functions replaced with the palm equivalents
int stripHTMLTags(char *sToClean, size_t size)
    {
        int i=0,j=0,k=0;
        int flag = 0; // 0: searching for < or & (& as in &bspn; etc), 1: searching for >, 2: searching for ; after &, 3: searching for </script>,</style>, -->
        char searchbuf[BUF_LEN] =  "";
        MemSet(temp, BUF_LEN, 0);

        while(i<size)
        {
            if(flag == 0)
            {
                if(sToClean[i] == '<')
                {
                    flag = 1;

                    temp[0] = '\0';
                    k=0; // track for <script>,<style>, <!-- --> etc
                }
                else if(sToClean[i] == '&')
                {
                    flag = 2;
                }
                else
                {
                    sToClean[j] = sToClean[i];
                    j++;
                }
            }
            else if(flag == 1)
            {
                temp[k] = sToClean[i];
                k++;
                temp[k] = '\0';

                //printf("DEBUG: %s\n",temp);

                if((0 == StrCompare(temp,"script")))
                {
                    flag = 3;

                    StrCopy(searchbuf,"</script>");
                    //printf("DEBUG: Detected %s\n",temp);

                    temp[0] = '\0';
                    k = 0;
                }
                else if((0 == StrCompare(temp,"style")))
                {
                    flag = 3;

                    StrCopy(searchbuf,"</style>");
                    //printf("DEBUG: Detected %s\n",temp);

                    temp[0] = '\0';
                    k = 0;
                }
                else if((0 == StrCompare(temp,"!--")))
                {
                    flag = 3;

                    StrCopy(searchbuf,"-->");
                    //printf("DEBUG: Detected %s\n",temp);

                    temp[0] = '\0';
                    k = 0;
                }

                if(sToClean[i] == '>')
                {
                    sToClean[j] = ' ';
                    j++;
                    flag = 0;
                }

            }
            else if(flag == 2)
            {
                if(sToClean[i] == ';')
                {
                    sToClean[j] = ' ';
                    j++;
                    flag = 0;
                }
            }
            else if(flag == 3)
            {
                temp[k] = sToClean[i];
                k++;
                temp[k] = '\0';

                //printf("DEBUG: %s\n",temp);
                //printf("DEBUG: Searching for %s\n",searchbuf);

                if(0 == StrCompare(&temp[0] + k - StrLen(searchbuf),searchbuf))
                {
                    flag = 0;
                    //printf("DEBUG: Detected END OF %s\n",searchbuf);

                    searchbuf[0] = '\0';
                    temp[0] = '\0';
                    k = 0;
                }
            }

            i++;
        }

        sToClean[j] = '\0';
        sToClean[j+1] = '\0';

        return j;
    }
