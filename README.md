# Pin-Sequencer
An Arduino sketch that implements one second granular pin sequences, with oneShot or repeating patterns

The sketch implements the sequence storage using PROGMEM.  The sketch could support up to 255 individual pins to sequence. Each sequence requires 4 bytes of RAM to hold index and duration info.  So if you actually created 255 sequences 1k of RAM would be consumed by the control array.

Alas, The Mega2560 only has 54 digital and 16 analog pins. So realistically only seventy pins can be sequenced.  But, you can create multiple sequences for the same pin, As long as only a single sequence is activate for one pin, it would work.  If you activate multiple sequences on the same pin, they are applied in order. 

As currently written (V0.1.0), `initIndex()` starts all repeating sequences.  If this is not wanted, each sequence can be individually started using `activateSequence(uint8_t sequence);` where sequence is in (0..LEDCount) for the sequence to start.

Each of the sequences is stored as an array of integers (int16_t) with the affected Arduino pin as the 'zeroth' element.  Each sequence element is the number of seconds for the pin to be HIGH or LOW, if the value is negative, the pin is driven LOW.  This array of time/polarity is terminated by a two element sequence. 0,0 marks this sequence as a 'OneShot' sequence.  It play's once and leaves the pin in the last specified state(HIGH or LOW).  If the termination sequence is 0,n where n is the index back into this sequence to 'repeat'. Typically this end marker is 0,1 which says repeat this entire sequence, starting with the first entry.

