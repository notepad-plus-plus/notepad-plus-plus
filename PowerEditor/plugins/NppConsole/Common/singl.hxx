/*
This file is part of Notepad++ console plugin.
Copyright ©2011 Mykhajlo Pobojnyj <mpoboyny@web.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

//singl.hxx


#ifndef _SINGL_HXX_
#define _SINGL_HXX_

template <class CLS> 
struct TSingl
{
	static CLS& Instance(bool destroy=false)
	{
	  static CLS *inst=0;
	  if (destroy) {
		delete inst;
		inst=0;
	  } 
	  else{
		if (!inst) {
			inst=new CLS();
		}
	  }
	  return *inst;
	}
};

// class SinglImpl : public TSingl<SinglImpl>
// {
	// friend struct TSingl<SinglImpl>;
	// SinglImpl(){}
	// ~SinglImpl(){}
	// public:
		// void TestFu()
		// {
		  // printf("\n TestFu \n");
		// }
// };

#endif //_SINGL_HXX_
