/*************************************************************************************************/
/**
	symboltable.h


	Copyright (C) Rich Talbot-Watkins 2007 - 2012

	This file is part of BeebAsm.

	BeebAsm is free software: you can redistribute it and/or modify it under the terms of the GNU
	General Public License as published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	BeebAsm is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
	even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with BeebAsm, as
	COPYING.txt.  If not, see <http://www.gnu.org/licenses/>.
*/
/*************************************************************************************************/

#ifndef SYMBOLTABLE_H_
#define SYMBOLTABLE_H_

#include <cassert>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>


class SymbolTable
{
public:

	static void Create();
	static void Destroy();
	static inline SymbolTable& Instance() { assert( m_gInstance != NULL ); return *m_gInstance; }

	void AddSymbol( const std::string& symbol, double value, bool isLabel = false, bool isStack = false );
	void ChangeSymbol( const std::string& symbol, double value );
	double GetSymbol( const std::string& symbol ) const;
	bool IsSymbolDefined( const std::string& symbol ) const;
	void RemoveSymbol( const std::string& symbol );
	bool IsStack( const std::string& symbol) const;
	bool IsEmptyStack( const std::string& symbol) const;
	void PushStackSymbol( const std::string& symbol, double value );
	void PopStackSymbol( const std::string& symbol );

	void Dump() const;


private:

	class Symbol
	{
	public:

		Symbol( double value, bool isLabel, bool isStack ) : m_value( value ), m_isLabel( isLabel ), m_isStack( isStack ) {}

		// SFTODO: INVOKING SETVALUE() ON A STACK SHOULD PROBABLY BE AN ERROR
		void SetValue( double d ) { m_value = d; }
		double GetValue() const;
		bool IsLabel() const { return m_isLabel; }
		bool IsStack() const { return m_isStack; }
		bool IsEmptyStack() const { return m_isStack && m_stack.empty(); }
		void PushStack( double d ) { m_stack.push_back( d ); }
		void PopStack();

	private:

		double	m_value;
		bool	m_isLabel;
		bool	m_isStack;
		std::vector<double> m_stack;
	};

	SymbolTable();
	~SymbolTable();

	std::map<std::string, Symbol>	m_map;

	static SymbolTable*				m_gInstance;
};



#endif // SYMBOLTABLE_H_
