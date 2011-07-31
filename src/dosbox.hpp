// Minimal header for OPL synth requirements
#ifndef DOSBOX_DOSBOX_H
#define DOSBOX_DOSBOX_H
#include <stdint.h>

typedef uintptr_t Bitu;
typedef  intptr_t Bits;
typedef   uint8_t Bit8u;
typedef    int8_t Bit8s;
typedef  uint16_t Bit16u;
typedef   int16_t Bit16s;
typedef  uint32_t Bit32u;
typedef   int32_t Bit32s;

#define INLINE inline
#define GCC_UNLIKELY

class MixerChannel {
	public:
	virtual void AddSamples_m32(Bitu samples, Bit32s *buffer) = 0;
	virtual void AddSamples_s32(Bitu samples, Bit32s *buffer) = 0;
};

#endif // DOSBOX_DOSBOX_H


#ifndef DOSBOX_ADLIB_H
#define DOSBOX_ADLIB_H

namespace Adlib {
class Handler {
	public:
	//Write an address to a chip, returns the address the chip sets
	virtual Bit32u WriteAddr( Bit32u port, Bit8u val ) = 0;
	//Write to a specific register in the chip
	virtual void WriteReg( Bit32u addr, Bit8u val ) = 0;
	//Generate a certain amount of samples
	virtual void Generate( MixerChannel* chan, Bitu samples ) = 0;
	//Initialize at a specific sample rate and mode
	virtual void Init( Bitu rate ) = 0;
	virtual ~Handler() {
	}
};
}

#endif // DOSBOX_ADLIB_H
