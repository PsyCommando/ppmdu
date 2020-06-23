#ifndef _15BITS_INTEGER_HPP
#define _15BITS_INTEGER_HPP
#include <cstdint>

namespace pmd2
{
//==========================================================================================================
//  15b signed integer support
//==========================================================================================================
	class int15_t
	{
	public:
		// --- Construction ---
		inline int15_t() :val(0) {}

		inline int15_t(const int15_t & cp)
		{
			this->operator=(cp);
		}

		inline int15_t & operator=(const int15_t & cp)
		{
			val = cp.val;
		}

		//
		inline explicit int15_t(uint16_t otherval)
		{
			this->operator=(otherval);
		}

		inline explicit int15_t(int otherval)
		{
			this->operator=(otherval);
		}

		inline int15_t(unsigned int otherval)
		{
			this->operator=(otherval);
		}

		inline int15_t & operator=(uint16_t otherval)
		{
			val = Convert16bTo15b(otherval);
		}

		inline int15_t & operator=(int otherval)
		{
			val = Convert16bTo15b(static_cast<unsigned int>(otherval));
		}

		inline int15_t & operator=(unsigned int otherval)
		{
			val = Convert16bTo15b(static_cast<unsigned int>(otherval));
		}

		// --- Operators ---

		//Logicals
		inline bool operator! ()const { return !val; }
		inline bool operator==(const int15_t & other)const { return val == other.val; }
		inline bool operator!=(const int15_t & other)const { return !operator==(other); }
		inline bool operator< (const int15_t & other)const { return val < other.val; }
		inline bool operator> (const int15_t & other)const { return val > other.val; }
		inline bool operator<=(const int15_t & other)const { return !operator>(other); }
		inline bool operator>=(const int15_t & other)const { return !operator<(other); }

		//Bitwises
		inline int15_t operator|(const int15_t & other)const { return (val | other.val); }
		inline int15_t operator&(const int15_t & other)const { return (val & other.val); }
		inline int15_t operator^(const int15_t & other)const { return (val ^ other.val); }
		inline int15_t operator~()const { return (~val) & Mask15b; }

		inline int15_t & operator|=(const int15_t & other) { return ((*this) = operator|(other)); }
		inline int15_t & operator&=(const int15_t & other) { return ((*this) = operator&(other)); }
		inline int15_t & operator^=(const int15_t & other) { return ((*this) = operator^(other)); }

		inline int15_t operator>>(unsigned int shiftamt)const { return (val >> shiftamt) & Mask15b; }
		inline int15_t operator<<(unsigned int shiftamt)const { return (val << shiftamt) & Mask15b; }
		inline int15_t & operator>>=(unsigned int shiftamt) { return ((*this) = operator>>(shiftamt)); }
		inline int15_t & operator<<=(unsigned int shiftamt) { return ((*this) = operator<<(shiftamt)); }

		//Arithmetics
		inline int15_t operator+(const int15_t & other)const { return (val + other.val) & Mask15b; }
		inline int15_t operator-(const int15_t & other)const { return (val - other.val) & Mask15b; }
		inline int15_t operator*(const int15_t & other)const { return (val * other.val) & Mask15b; }
		inline int15_t operator/(const int15_t & other)const { return (val / other.val) & Mask15b; }
		inline int15_t operator%(const int15_t & other)const { return (val % other.val) & Mask15b; }

		inline int15_t & operator+=(const int15_t & other) { return ((*this) = operator+(other)); }
		inline int15_t & operator-=(const int15_t & other) { return ((*this) = operator-(other)); }
		inline int15_t & operator*=(const int15_t & other) { return ((*this) = operator*(other)); }
		inline int15_t & operator/=(const int15_t & other) { return ((*this) = operator/(other)); }
		inline int15_t & operator%=(const int15_t & other) { return ((*this) = operator%(other)); }

		inline int15_t & operator++() { return this->operator+=(1); }
		inline int15_t   operator++(int) { int15_t tmp(*this); operator++(); return tmp; } //post increment
		inline int15_t & operator--() { return this->operator-=(1); }
		inline int15_t   operator--(int) { int15_t tmp(*this); operator++(); return tmp; } //post decrement

		// --- Cast ---
		inline operator uint16_t()const { return Convert15bTo16b(val); }
		inline operator int16_t ()const { return Convert15bTo16b(val); }

		// --- Conversion ---
		static inline int16_t Convert15bTo16b(uint16_t value)
		{
			return (value >= 0x4000u) ? (value | 0xFFFF8000u) : (value & 0x3FFFu);
		}

		//!#TODO: Double check this!!
		static inline uint16_t Convert16bTo15b(uint16_t value)
		{
			if (value == 0)
				return 0;
			else if (static_cast<int16_t>(value) < 0)
				return (value & 0x3FFFu) | 0x4000u;     //If the value was negative, set the negative bit
			return value & 0x3FFFu;
		}

	private:
		uint16_t val;
		static const uint16_t Mask15b = 0x7FFFu;
	};
};

#endif