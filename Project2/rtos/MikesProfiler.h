
#define PROFILECODE 1

#define PROFILE_DATADIRECTIONREGISTER 	DDRC
#define PROFILE_OUTPORT 				PORTC
#define PROFILE_OUT_PIN0 				PINC0
#define PROFILE_OUT_PIN1 				PINC1
#define PROFILE_OUT_PIN2 				PINC2
#define PROFILE_OUT_PIN3 				PINC3
#define PROFILE_OUT_PIN4 				PINC4
#define PROFILE_OUT_PIN5 				PINC5
#define PROFILE_OUT_PIN6 				PINC6
#define PROFILE_OUT_PIN7 				PINC7

#if PROFILECODE

#define InitializeLogicAnalyzerProfiler() { PROFILE_DATADIRECTIONREGISTER |= 0xff; }
#define EnableProfileSample1() { PROFILE_OUTPORT |= (_BV(PROFILE_OUT_PIN0)); }
#define EnableProfileSample2() { PROFILE_OUTPORT |= (_BV(PROFILE_OUT_PIN1)); }
#define EnableProfileSample3() { PROFILE_OUTPORT |= (_BV(PROFILE_OUT_PIN2)); }
#define EnableProfileSample4() { PROFILE_OUTPORT |= (_BV(PROFILE_OUT_PIN3)); }
#define EnableProfileSample5() { PROFILE_OUTPORT |= (_BV(PROFILE_OUT_PIN4)); }
#define EnableProfileSample6() { PROFILE_OUTPORT |= (_BV(PROFILE_OUT_PIN5)); }
#define EnableProfileSample7() { PROFILE_OUTPORT |= (_BV(PROFILE_OUT_PIN6)); }
#define EnableProfileSample8() { PROFILE_OUTPORT |= (_BV(PROFILE_OUT_PIN7)); }


#define DisableProfileSample1() { PROFILE_OUTPORT &= ~(_BV(PROFILE_OUT_PIN0)); }
#define DisableProfileSample2() { PROFILE_OUTPORT &= ~(_BV(PROFILE_OUT_PIN1)); }
#define DisableProfileSample3() { PROFILE_OUTPORT &= ~(_BV(PROFILE_OUT_PIN2)); }
#define DisableProfileSample4() { PROFILE_OUTPORT &= ~(_BV(PROFILE_OUT_PIN3)); }
#define DisableProfileSample5() { PROFILE_OUTPORT &= ~(_BV(PROFILE_OUT_PIN4)); }
#define DisableProfileSample6() { PROFILE_OUTPORT &= ~(_BV(PROFILE_OUT_PIN5)); }
#define DisableProfileSample7() { PROFILE_OUTPORT &= ~(_BV(PROFILE_OUT_PIN6)); }
#define DisableProfileSample8() { PROFILE_OUTPORT &= ~(_BV(PROFILE_OUT_PIN7)); }

#define ProfileBlip(a) { 			\
EnableProfileSample##a(); 			\
	volatile count = 0;				\
	for( int i = 0; i < 2; i++ )	\
	{								\
		count++;					\
	}								\
DisableProfileSample##a(); 			\
}


#define Profile1() ProfileBlip(1)
#define Profile2() ProfileBlip(2)
#define Profile3() ProfileBlip(3)
#define Profile4() ProfileBlip(4)
#define Profile5() ProfileBlip(5)
#define Profile6() ProfileBlip(6)
#define Profile7() ProfileBlip(7)
#define Profile8() ProfileBlip(8)


#endif
