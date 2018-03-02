//
//  main.c
//  Reciproke Counter
//
//  Created by Frans Evenblij on 18/02/2018.
//  Copyright © 2018 Frans Evenblij. All rights reserved.
//
//  Based on Elektuur 255: uP gestuurde frequentiemeter
//

//#define TESTING yes /* not automatically defined by makefile when run from XCode */

// Includes
// {{{

#include <stdio.h>
#include <stdint.h>

#ifdef TESTING
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <ctype.h>
#endif

// }}}
// Constants
// {{{
#define TIMEBASE_FREQUENCY 10000000L
#define FALSE 0
#define TRUE -11

// bit 0    reserved
// bit 1    0=6 digits, 1=7 digits
// bit 2..4 0=frequency, 1=period, 2=pulsehi, 3=pulselow, 4=event
// bit 5    0= <100Mhz 1= >100MHz
// bit 6    0= analog, 1=digital
// bit 7    reserved

#define MASK_DIGITS 0x02
#define P6DIGITS    0x00
#define P7DIGITS    0x02 

#define MASK_MODE   0x1C
#define FREQUENCY   0x00
#define PERIOD      0x04
#define PULSEHI     0x08
#define PULSELO     0x0C
#define EVENT       0x10

#define MASK_INPUT  0x60
#define MHZ         0x00
#define GHZ         0x20
#define DIGITAL     0x40

#define MEASURCNT 7
char *measurements[] = { 
                        "| Meeting |",
                        "|         |",
                        "| Freq    |", 
                        "| Per     |", 
                        "| pHi     |", 
                        "| pLo     |",
                        "| Evt     |"};

#define MODECNT 5
char *ModeString[] = {  "Freq     ", 
                        "Period   ", 
                        "Pos Pulse", 
                        "Neg Pulse",
                        "Events   " };
#define UNITCNT  10
char *UnitString[] = {  "mHz  ", "Hz   ", "kHz  ", "MHz  ", "GHz  ", 
                        "ns   ", "us   ", "ms   ", "sec  ", "     " };
#define INPUTCNT 5
char *InputString[] = {  
                        "| Input   |",
                        "|         |",
                        "| 1.2 GHz |", 
                        "| Digital |", 
                        "| 100 MHz |" };

// {{{ User Commands

#define cFREQUENCY  'f'
#define cPERIOD     'p'
#define cPULSEHI    'h'
#define cPULSELO    'l'
#define cEVENT      'e'
#define iGHZ        'g'
#define iMHz        'm'
#define iDIGITAL    'd'
#define iP6DIGITS   '6'
#define iP7DIGITS   '7'
#define bSETUP      'b'
#define bHOLD       'c'

#ifdef TESTING
#define bQUIT       'q'
#endif

// }}}            
// }}}
// Globals
// {{{
uint8_t     ExitMainLoop=FALSE;
uint16_t    CommandRegisterChanged=TRUE;
uint8_t     CommandRegister=P6DIGITS + FREQUENCY + MHZ;
uint64_t    GateTimeTest;
uint64_t    GateTimeFinal;
uint64_t    TimeBasePulsTest=-2;
uint64_t    TimeBasePulsFinal=-2;
uint64_t    CounterValue=0;
uint64_t    DisplayValue=1;
uint64_t    PortPrescaler=0;
uint8_t     DividerSetting=0;
uint32_t    PrevValue=0;
int         Precision=6;
uint32_t    OurTime=0;

#ifdef TESTING
uint64_t    InputSignal = -5;

int         YTop;
int         XTop;

int         GetcBuffer;
int         GetcAvail=FALSE;

struct termios orig_termios;

#define DISPLAY_WIDTH 80
#define DISPLAY_HEIGHT 24
#endif

// }}}
// Prototypes
// {{{
void mainLoop(void);
void init(void);
void initMultiplexers(void);
void initDisplay(void);
void initMenu(void);
void initMeasuring(void);
void setupCommandExecution(void);
void sampleMeasurement(void);
void finalMeasurement(void);
void setupDisplay(void);
void getCounterValue(void);
uint8_t getDividerSetting(uint64_t n);
void calculateDisplayValue(void);
void showValueOnDisplay(void);
void updateAppClock(void);

void layo_ShowValue(uint32_t value, short decimalPosition);
void layo_BackGround(void);

void layo_bg_title(char *str);
void layo_bgmodemenu(void);
void layo_bg_inputs(void);
void layo_bg_buttons_main(void);
void layo_bg_units(char *str);
void layo_bg_mode(char *str);

void deSetCursorPosition(short row, short col);
void deSetColor(int,int);
void deClearScreen(void);
void deClearColor(void);

#ifdef TESTING
int  select(int, fd_set * __restrict, fd_set * __restrict, fd_set * __restrict, struct timeval * __restrict);
void debug(void);
#endif

// }}}

// IO Scaffolding
#ifdef TESTING
// {{{ Scaffolding test code

// {{{ Output Scaffolding

// {{{ ttyControl

// {{{ void ttySetCursorPosition(short row, short col)

void ttySetCursorPosition(short row, short col)
{
    printf("\033[%d;%df",row+1, col+1);
}

// }}}
// {{{ void ttyClearScreen(void)

void ttyClearScreen(void)
{
    printf("\033[0m");  // reset terminal
    printf("\033[2J");  // clear the screen
    printf("\033[H");   // home the cursor
    // printf("\033[?1h");
}

// }}}
// {{{ void ttySetColor(int fg, int bg)

void ttySetColor(int fg, int bg)
{
    printf("\033[%02dm",fg);
    printf("\033[%02dm",bg);
}

// }}}
// {{{ void ttyClearColor(void)

void ttyClearColor(void)
{
    printf("\033[0m");
}

// }}}

// }}}
// {{{ Display emulator

// {{{ deNL(void)

void deNL(void)
{
    printf("\n");
    printf ("\r");
}

// }}}
// {{{ deSetCursorPosition(short row, short col)

void deSetCursorPosition(short row, short col)
{
    ttySetCursorPosition(row+YTop, col+XTop);
}

// }}}
// {{{ deClearColor(void)

void deClearColor(void)
{
    ttyClearColor();
}

// }}}
// {{{ deTop(void)

void deTop(void)
{
    deSetCursorPosition(0,0);
}

// }}}
// {{{ deData(char c)

void deData(char c)
{
    printf("%c",c);
    //cpos++;
    //if (cpos==DISPLAY_WIDTH)
    //    deSetCursorPosition(2,1);
}

// }}}
// {{{ deClearScreen(void)

void deClearScreen(void)
{
    ttyClearScreen();
}

// }}}
// {{{ deCmd(char c)

void deCmd(char c)
{
    /*
       short pos;
       short row;
       if (c == dispCLEAR)
       {
       deClearScreen();
       }

       if ((c & 0xFE) == dispHOME)
       {
       deTop();
       }

       if ((c & 0x80) == dispDDRA)
       {
       pos = c & 0x0F;
       row = (c & 0x40) ? 1 : 0;
       deSetCursorPosition(row,pos);
       }
     */
}

// }}}
// {{{ deFrame(void)

void deLine(short w)
{
    short i;
    printf("+");
    for (i=1; i<w; i++)
        printf("-");
    printf("+\n"); deNL();
}

void deFrame(void)
{
    short y;

    // cursor off
    printf("\033[?25l");

    deSetCursorPosition(-1, -1);
    deLine(DISPLAY_WIDTH+1);
    for (y=0; y<DISPLAY_HEIGHT; y++)
    {
        deSetCursorPosition(y, -1);
        printf("|");
        deSetCursorPosition(y,DISPLAY_WIDTH);
        printf("|");
    }
    deSetCursorPosition(DISPLAY_HEIGHT, -1);
    deLine(DISPLAY_WIDTH+1);
}

// }}}
// {{{ deSetColor(int fg, int bg)

void deSetColor(int fg, int bg)
{
    ttySetColor(fg,bg);
}

// }}}

// }}}
// {{{ char* yesno(short boolean)

char* yesno(short boolean)
{
    // green -> light gray
    static char myes[]="\033[32myes\033[37m";
    // red -> light gray
    static char mno[] ="\033[31m no\033[37m";
    return (boolean ? myes : mno);
}

// }}}

// }}}
// {{{ Input Scaffolding

// {{{ Reset console

void reset_conio_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
}

// }}}
// {{{ Set console

void set_conio_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    tcgetattr(0, &new_termios);

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_conio_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

// }}}

// {{{ FHEkbhit
// return 0 on no input on stdin
// return <0 on error
// return 1 on input available on stdin
int FHEkbhit()
{
    struct timeval tv = { 0L, 1000L };  // 0L 0L means Do not wait.
    fd_set fds;

    FD_ZERO(&fds);      // clear all file descriptors in the array
    FD_SET(0, &fds);    // add STDIN to the array of file descriptors
    // 1 is the number of file descriptors
    // &fds is the file descriptor array for reading
    // NULL and NULL specifu fds for writing and exception reporting
    // tv is the timeout value
    return select(1, &fds, NULL, NULL, &tv);
}

// }}}
// {{{ short FHEgetchar() (blocking)

short FHEgetchar() 
{
    short r;
    char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}

// }}} 
/*
// {{{ char FHEgetc (non blocking)

// non blocking getkeyboard function
// return FALSE if no character is available
char FHEgetc(void)
{
    char c;

    if (GetcAvail)
    {
        c = GetcBuffer;
        GetcBuffer=0;
    } else
    {
        if (FHEkbhit())
        {
            c = FHEgetchar();
        } else
        {
            c = FALSE;
        }
    }
    GetcAvail = FALSE;
    return c;
}

// }}}
// {{{ void FHEungetc(char c)

void FHEungetc(char c)
{
    GetcBuffer = c;
    GetcAvail = TRUE;
}

// }}}
*/
// }}}

// {{{ void setCommandRegister(uint8_T value, uint8_t mask)

void setCommandRegister(uint8_t mask, uint8_t value)
{
    CommandRegister &= 0-1-mask;
    CommandRegister |= value;
    CommandRegisterChanged = TRUE;
//    deSetCursorPosition(24,1);
//    printf("mask=%04x, value=%04x",(0-1-mask)&0xFFFF, value);
//    deSetCursorPosition(25,1);
//    printf("mask=%04x, comdr=%04x",mask, CommandRegister);
}

// }}}
// {{{ char parseCommand(char c)

void parseCommand(char c)
{
    c = tolower(c);

    switch (c)
    {
        // Meeting
        case cFREQUENCY : // frequency
            setCommandRegister(MASK_MODE, FREQUENCY);
            break;

        case cPERIOD    : // periode
            setCommandRegister(MASK_MODE, PERIOD);
            break;

        case cPULSEHI   : // positive pulse
            setCommandRegister(MASK_MODE, PULSEHI);
            break;

        case cPULSELO   : // negative pulse
            setCommandRegister(MASK_MODE, PULSELO);
            break;

        case cEVENT     : // event
            setCommandRegister(MASK_MODE, EVENT);
            break;

            // Input
        case iMHz       : // MHz
            setCommandRegister(MASK_INPUT, MHZ);
            break;

        case iGHZ       : // GHz
            setCommandRegister(MASK_INPUT, GHZ);
            break;

        case iDIGITAL   : // Digital
            setCommandRegister(MASK_INPUT, DIGITAL);
            break;

            // Precision 
        case iP6DIGITS  : // 6 digits precision
            setCommandRegister(MASK_DIGITS, P6DIGITS);
            DisplayValue = 145750;
            break;

        case iP7DIGITS  : // 7 digits precision
            setCommandRegister(MASK_DIGITS, P7DIGITS);
            DisplayValue = 1457500;
            break;

            // Buttons
        case bSETUP     : // Setup
            break;

        case bHOLD      : // Hold
            break;

#ifdef TESTING
        case 3          :
        case 27         :
        case bQUIT      : // End The Simulator
            ExitMainLoop = TRUE;
            break;
#endif
    }
}

// }}}

// }}}
#endif

// {{{ Initialisation Code

// {{{ void init(void)

void init(void)
{
#ifdef TESTING
    set_conio_mode();
#endif

    initDisplay();
    initMultiplexers();
    initMenu();
    initMeasuring();
}

// }}}
// {{{  void outit(void)

void outit(void)
{
#ifdef TESTING
    deSetCursorPosition(33,1); 
    printf("\r\nReciproke Counter Finished\r\n");
#endif
}

// }}}

void initMultiplexers(void)
{/*{{{*/
}/*}}}*/

void initDisplay(void)
{/*{{{*/
    deClearScreen();
    layo_BackGround();
}/*}}}*/

void initMenu(void)
{/*{{{*/
    layo_bgmodemenu();
    layo_bg_inputs();
    layo_bg_buttons_main();
}/*}}}*/

void initMeasuring(void)
{/*{{{*/
}/*}}}*/

// }}}
// {{{ int main(void)

int main(void)
{
    init();
    while (!ExitMainLoop)
    {
        mainLoop();
        debug();
    }
    outit();
}

// }}}
// {{{ void mainLoop(void)
// {{ MainLoop code

void mainLoop(void)
{
#ifdef TESTING
    static uint32_t prevTime;
    char c;
    if (FHEkbhit())
    {
        c = FHEgetchar();
        deSetCursorPosition(20,1);
        parseCommand(c);
    }
    //printf("a\n");
    updateAppClock();

#endif
    //printf("b\n");
    if (CommandRegisterChanged)
    {
        CommandRegisterChanged = FALSE;
        setupCommandExecution();
        //printf("c\n");
        setupDisplay();
        //printf("d\n");
    }
#ifdef TESTING
    if ((OurTime - prevTime) > 1)   // larger then 1 sec
    {
        prevTime = OurTime;
        InputSignal = 48000L;
#endif
        sampleMeasurement();
        finalMeasurement();
        getCounterValue();
        calculateDisplayValue();
        showValueOnDisplay();
#ifdef TESTING
    }
#endif
}

// }}
// }}}

// {{{ void setupCommandExecution(void)

void setupCommandExecution(void)
{
    uint16_t mode;

    mode = CommandRegister & MASK_MODE;
    switch (mode)
    {
        case cFREQUENCY :

            break;

        case cPERIOD    :

            break;

        case cPULSEHI   :

            break;

        case cPULSELO   :

            break;

        case cEVENT     :

            break;

        case iGHZ       : 

            break;

        case iMHz       : 

            break;

        case iDIGITAL   : 

            break;

        case iP6DIGITS  :
            if ((CommandRegister & MASK_DIGITS) == FALSE)
                break;

        case iP7DIGITS  :
            if ((CommandRegister & MASK_DIGITS) == TRUE)
                break;
    }
    CommandRegisterChanged = FALSE;
}

// }}}            
// {{{ void setupDisplay(void)

void setupDisplay(void)
{
    layo_BackGround();
}

// }}}

// {{{ void sampleMeasurement(void);

void sampleMeasurement(void)
{
    printf("1a\n");
    PortPrescaler = 2; // (for the first sample measurement
#ifdef TESTING
    {
        // GateFreq = InputSignal / PortPrescaler; 
        // GateTime = GatePeriod / 2
        // GateTime = 1 / (GateFreq * 2)
        // CounterValue = TIMEBASE_FREQUENCY * GateTime
        // CounterValue = TIMEBASE_FREQUENCY / (GateFreq * 2)
        // CounterValue = TIMEBASE_FREQUENCY / ((InputSignal/PortPrescaler) * 2)
        CounterValue = (TIMEBASE_FREQUENCY * PortPrescaler) / (InputSignal * 2);
        printf("1b\n");
        TimeBasePulsTest = CounterValue;
        printf("1c\n");
        DividerSetting = getDividerSetting(TimeBasePulsTest);
        printf("1d\n");
    }

#endif
}

// }}}
// {{{ void finalMeasurement(void);

void finalMeasurement(void)
{
    printf("2a\n");
    PortPrescaler = 1 << DividerSetting;
    printf("%llu\n" ,PortPrescaler);
    printf("2b\n");
    CounterValue =  (TIMEBASE_FREQUENCY * PortPrescaler);
    printf("2c\n");
    uint64_t tmp = (InputSignal * 2);
    printf("2d\n");
    CounterValue = CounterValue / tmp;
    printf("2e\n");
    TimeBasePulsFinal = CounterValue;
    printf("2f\n");
}

// }}}
// {{{ void getCounterValue(void)

void getCounterValue(void)
{
    CounterValue = TimeBasePulsFinal;
}

// }}}
// {{{ void calculateDisplayValue(void)

void calculateDisplayValue(void)
{
    printf("3a\n");
    DisplayValue = (TIMEBASE_FREQUENCY * PortPrescaler);
    printf("3b\n");
    //DisplayValue =  DisplayValue / TimeBasePulsFinal;
    printf("3c\n");
}

// }}}
// {{{ void showValueOnDisplay(void)

void showValueOnDisplay(void)
{

    printf("4\n");
    //if (PrevValue != DisplayValue)
    {
        layo_ShowValue(DisplayValue,3);
        PrevValue = DisplayValue;
    }
}

// }}}

// {{{ uint8_t getDividerSetting(pulses)

uint8_t getDividerSetting(uint64_t pulses)
{
    uint8_t  n = 0;
    uint64_t divided;
    do  
    {
        n++;
        divided  = pulses << n;
    }
    while ((divided < 1000000) && (n<31));
    return n;
}

// }}}
// {{{ char *getMode(void)

char *getMode(void)
{
    int m = (CommandRegister & MASK_MODE) >> 2;
    return ModeString[m];
}

// }}}
// {{{ char *getUnits(void)

char *getUnits(void)
{
    int u;
    switch (CommandRegister & MASK_MODE) 
    {
        case FREQUENCY :
            u=3; 
            break;
        case PERIOD :
        case PULSELO :
        case PULSEHI :
            u=7; 
            break;
        case EVENT :
            u=9; 
            break;
    }
    return UnitString[u];
}

// }}}
// {{{ uint32_t sysClock(void)

uint32_t sysClock(void)
{       
    register uint32_t rv;
#ifdef TESTING
    // posix clock works in uS
    // dividing by 10000 converts to centiseconds
    rv = clock() / 10000L;
#else
    // Atmel timer setup uses approximate centiseconds
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        rv = IRQ_Ticks;
    }
#endif
    return rv;
}   

// }}}
// {{{ updateAppClock();

void updateAppClock(void)
{
    OurTime = sysClock();
}

// }}}

// {{{ Layout Module

#define VALUELINE 6

// {{{ void layo_ShowValue(uint32_t value, int decPos)

void layo_ShowValue(uint32_t value, short decimalPosition)
{
    deSetCursorPosition(VALUELINE,30); 
    printf("%4d.%d", value/1000, value%1000);
    printf(" ");
}

// }}}
// {{{ void layo_BackGround(void)

void layo_BackGround(void)
{
    layo_bg_title("PA3BJI Reciproke Counter");
    layo_bgmodemenu();
    layo_bg_inputs();
    layo_bg_buttons_main();
    layo_bg_mode(getMode());
    layo_bg_units(getUnits());
    printf("\n");
}


// }}}
// {{{ void layo_bg_title(char *title)

void layo_bg_title(char *title)
{
    deSetCursorPosition(1,1); 
    printf("%s",title);
}

// }}}
// {{{ void layo_bgmodemenu(void)

void layo_bgmodemenu(void)
{
    short row=3;
    short col=1;
    deSetColor(97,44);
    deSetCursorPosition(row,col); 
    printf("%s", measurements[0]);
    deClearColor();
    for (int i=2; i<MEASURCNT; i++)
    {
        row++;
        deSetCursorPosition(row,col); 
        printf("%s",measurements[1]);
        row++;
        deSetCursorPosition(row,col); 
        printf("%s",measurements[i]);
    }
}

// }}}
// {{{ void layo_bg_inputs(void)

void layo_bg_inputs(void)
{
    short row=3;
    short col=50;
    deSetColor(97,44);
    deSetCursorPosition(row,col); 
    printf("%s", InputString[0]);
    deClearColor();
    row++;
    deSetCursorPosition(row,col); 
    printf("%s", InputString[1]);
    for (int i=2; i<INPUTCNT; i++)
    {
        row++;
        deSetCursorPosition(row,col); 
        printf("%s",InputString[i]);    
        for (int j=0; j<2; j++)
        {
            row++;
            deSetCursorPosition(row,col); 
            printf("%s", InputString[1]);
        }
    }
}

// }}}
// {{{ void layo_bg_buttons_main(void)

void layo_bg_buttons_main(void)
{
    deSetCursorPosition(16, 3); 
    printf("Setup");
    deSetCursorPosition(16,25); 
    printf("6/7 digts");
    deSetCursorPosition(16,51); 
    printf("hold/Cont");
}

// }}}
// {{{ void layo_bg_mode(char *modeStr)

void layo_bg_mode(char *modeStr)
{
    deSetCursorPosition(VALUELINE,18); 
    printf(" %s",modeStr);
}
// }}}
// {{{ void layo_bg_units(char *unitStr)

void layo_bg_units(char *unitStr)
{
    deSetCursorPosition(VALUELINE,40); 
    printf(" %s",unitStr);
}
// }}}

// }}}

// {{{ void printCmdReg()

void printCmdReg(void)
{
    uint8_t b = CommandRegister & 0xFF;
    for (int k=0; k<8; k++)
    {
        if ((b&0x80)==0x80) 
            printf("1");
        else 
            printf("0");
        if (k==0) printf(" ");
        if (k==2) printf(" ");
        if (k==5) printf(" ");
        if (k==6) printf(" ");
        b = b<<1;
    }
}

// }}}
// {{{ void debug(void)

void debug(void)
{
    int topDbg=20;
    deSetCursorPosition(topDbg++,1); printf("CmdReg=$%04X"           , CommandRegister);
    deSetCursorPosition(topDbg++,1); printf("CmdReg=");                printCmdReg();
    deSetCursorPosition(topDbg++,1); printf("OurTime=%d sec"         , OurTime); 
    deSetCursorPosition(topDbg++,1); printf("CounterValue=%8llu             " , CounterValue); 
    deSetCursorPosition(topDbg++,1); printf("DisplayValue=%8llu             " , DisplayValue); 
    deSetCursorPosition(topDbg++,1); printf("PortPrescaler=%8llu            " , PortPrescaler); 
    deSetCursorPosition(topDbg++,1); printf("DividerSetting=%d              " , DividerSetting); 

    deSetCursorPosition(topDbg++,1); printf("InputSignal=%8llu              " , InputSignal); 

    deSetCursorPosition(topDbg++,1); printf("GateTime Test=%8llu            " , GateTimeTest); 
    deSetCursorPosition(topDbg++,1); printf("TimeBasePulsTest=%8llu         " , TimeBasePulsTest); 


    deSetCursorPosition(topDbg++,1); printf("GateTime Final=%8llu           " , GateTimeFinal); 
    deSetCursorPosition(topDbg++,1); printf("TimeBasePulsFinal=%8llu        " , TimeBasePulsFinal); 

    deSetCursorPosition(topDbg++,1); printf("intermediate=%8llu             " , PortPrescaler*TIMEBASE_FREQUENCY); 

}

// }}}

// {{{
// }}}
// vi: ts=4 et foldmethod=marker sw=4
// EOF
