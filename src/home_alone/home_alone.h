
#ifndef _HOMEALONE_h
#define _HOMEALONE_h

// Define Debug Macros ----- A mofification of Adreas Spiess Macros ---------
#if DEBUGLEVEL >= 0
#define DEBUGPRINT0(...) Serial.print(__VA_ARGS__)
#define DEBUGPRINTLN0(...) Serial.println(__VA_ARGS__)
#else
#define DEBUGPRINT0(...)
#define DEBUGPRINTLN0(...)
#endif

#if DEBUGLEVEL >= 1
#define DEBUGPRINT1(...) Serial.print(__VA_ARGS__)
#define DEBUGPRINTLN1(...) Serial.println(__VA_ARGS__)
#else
#define DEBUGPRINT1(...)
#define DEBUGPRINTLN1(...)
#endif

#if DEBUGLEVEL >= 2
#define DEBUGPRINT2(...) Serial.print(__VA_ARGS__)
#define DEBUGPRINTLN2(...) Serial.println(__VA_ARGS__)
#else
#define DEBUGPRINT2(...)
#define DEBUGPRINTLN2(...)
#endif

#if DEBUGLEVEL >= 3
#define DEBUGPRINT3(...) Serial.print(__VA_ARGS__)
#define DEBUGPRINTLN3(...) Serial.println(__VA_ARGS__)
#else
#define DEBUGPRINT3(...)
#define DEBUGPRINTLN3(...)
#endif
// Ende degugging stuff ------------------------------

// ------ defines für Intervalls gültig wenn debug on ist
#define STATE_LEAVE_TEST 120         // in seconds for test debug
#define UPLOAD_INTERVALL_TEST 300    // in seconds for test debug
#define PUSHOVER_INTERVALL_TEST 900  // in seconds for test debug
#define HOURSBETWEENNOMOV 1          // 1 hour

char bufpush[80];  // buffer for puhover meldung 

enum display_show {
  NOSHOW,  // 
  NORMAL,
  START,
  ZEIT,
  REBOOT,
  WIFI
};
#endif

