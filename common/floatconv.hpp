//@	{
//@	 "targets":[{"name":"floatconv.hpp","type":"include"}]
//@	,"dependencies_extra":[{"ref":"floatconv.o","rel":"implementation"}]
//@	}

#ifndef ANJA_FLOATCONV_HPP
#define ANJA_FLOATCONV_HPP

namespace Anja
	{
	bool convert(const char* string,double& result);

	inline bool convert(const char* string,float& result)
		{
		double temp;
		auto ret=convert(string,temp); //This is cheating...
		result=temp;
		return ret;
		}

	inline double convert(const char* string)
		{
		double ret;
		if(!convert(string,ret))
			{throw "Invalid number format";}
		return ret;
		}
	}

#endif
