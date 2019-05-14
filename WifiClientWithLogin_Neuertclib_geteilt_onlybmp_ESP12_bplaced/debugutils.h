#define DEBUG 1

#ifdef DEBUG
  #define INIT_DEBUG_PRINT(x)  {Serial.begin (115200); delay(100);}
#else
  #define INIT_DEBUG_PRINT(x)
#endif


#ifdef DEBUG
 #define DEBUG_PRINT(x)  Serial.println (x)
 #define DEBUG_PRINTF(...) Serial.printf( __VA_ARGS__ )
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTF(...)
#endif
