/*************************************************************************************************/
/**
	discimage.cpp


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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include "discimage.h"
#include "asmexception.h"
#include "globaldata.h"

using namespace std;


/*************************************************************************************************/
/**
	DiscImage::DiscImage()

	DiscImage constructor
*/
/*************************************************************************************************/
DiscImage::DiscImage( const char* pOutput, const char* pInput )
	:	m_outputFilename( pOutput ),
		m_inputFilename( pInput ),
		m_discImagePtr( 0 )
{
	// make space for the catalogue
	m_aDiscImage.resize( 0x200 );

	// open and load input file if necessary

	if ( pInput != NULL )
	{
		std::ifstream inputFile;
		inputFile.open( pInput, ios_base::in | ios_base::binary );

		if ( !inputFile )
		{
			throw AsmException_FileError_OpenDiscSource( pInput );
		}

		if ( !inputFile.read( reinterpret_cast< char* >( &m_aDiscImage[ 0 ] ), 0x200 ) )
		{
			throw AsmException_FileError_ReadDiscSource( pInput );
		}

		// copy the disc contents to the output file

		int endSectorAddr;

		if ( m_aDiscImage[ 0x105 ] > 0 )
		{
			int sectorAddrOfLastFile	= m_aDiscImage[ 0x10F ] +
										  ( ( m_aDiscImage[ 0x10E ] & 0x03 ) << 8 );

			int lengthOfLastFile		= m_aDiscImage[ 0x10C ] +
										  ( m_aDiscImage[ 0x10D ] << 8 ) +
										  ( ( m_aDiscImage[ 0x10E ] & 0x30 ) << 12 );

			endSectorAddr = sectorAddrOfLastFile + ( ( lengthOfLastFile + 0xFF ) >> 8 );
		}
		else
		{
			endSectorAddr = 2;
		}

		inputFile.seekg( 0, ios::end );
		int length = static_cast< int >( inputFile.tellg() );
		inputFile.seekg( 0, ios::beg );

		assert( length >= endSectorAddr * 0x100 );

		char sector[ 0x100 ];

		for ( int sect = 0; sect < endSectorAddr; sect++ )
		{
			if ( !inputFile.read( sector, 0x100 ) )
			{
				throw AsmException_FileError_ReadDiscSource( pInput );
			}

			Write( sector, 0x100 );
		}

		inputFile.close();
	}
	else
	{
		// generate a blank catalog

		m_aDiscImage[ 0x106 ] = 0x03 | ( ( GlobalData::Instance().GetDiscOption() & 3 ) << 4);
		m_aDiscImage[ 0x107 ] = 0x20;

		const std::string& title = GlobalData::Instance().GetDiscTitle();
		strncpy( reinterpret_cast< char* >( &m_aDiscImage[ 0 ] ), title.substr(0, 8).c_str(), 8);
		if ( title.length() > 8 )
		{
			strncpy( reinterpret_cast< char* >( &m_aDiscImage[ 0x100 ] ), title.substr(8, 4).c_str(), 4);
		}

		// add in a boot file

		if ( GlobalData::Instance().GetBootFile() != NULL )
		{
			char pPlingBoot[ 64 ];
			strcpy( pPlingBoot, "*BASIC\r*RUN " );
			strcat( pPlingBoot, GlobalData::Instance().GetBootFile() );
			strcat( pPlingBoot, "\r" );

			AddFile( "!Boot", reinterpret_cast< unsigned char* >( pPlingBoot ), 0, 0xFFFFFF, strlen( pPlingBoot ) );

			m_aDiscImage[ 0x106 ] = 0x33;		// force *OPT to 3 (EXEC)
		}
	}

}



/*************************************************************************************************/
/**
	DiscImage::~DiscImage()

	DiscImage destructor
*/
/*************************************************************************************************/
DiscImage::~DiscImage()
{
}



/*************************************************************************************************/
/**
	DiscImage::Save()
*/
/*************************************************************************************************/
void DiscImage::Save()
{
	std::ofstream outputFile;
	outputFile.open( m_outputFilename, ios_base::out | ios_base::binary | ios_base::trunc );

	if ( !outputFile )
	{
		throw AsmException_FileError_OpenDiscDest( m_outputFilename );
	}

	if ( !outputFile.write( reinterpret_cast< char* >( &m_aDiscImage[ 0 ]), m_aDiscImage.size() ) )
	{
		throw AsmException_FileError_WriteDiscDest( m_outputFilename );
	}

	outputFile.close();
}



/*************************************************************************************************/
/**
	DiscImage::AddFile()
*/
/*************************************************************************************************/
void DiscImage::AddFile( const char* pName, const unsigned char* pAddr, int load, int exec, int len )
{
	char dirName = '$';

	if ( strlen( pName ) > 2 && pName[ 1 ] == '.' )
	{
		dirName = pName[ 0 ];
		pName += 2;
	}

	if ( strlen( pName ) > 7 )
	{
		// Bad name
		throw AsmException_FileError_BadName( m_outputFilename );
	}

	if ( m_aDiscImage[ 0x105 ] == 31*8 )
	{
		// Catalog full
		throw AsmException_FileError_TooManyFiles( m_outputFilename );
	}

	// Check the file doesn't already exist

	for ( int i = m_aDiscImage[ 0x105 ]; i > 0; i -= 8 )
	{
		bool bTheSame = true;

		for ( size_t j = 0; j < strlen( pName ); j++ )
		{
			if ( toupper( pName[ j ] ) != toupper( m_aDiscImage[ i + j ] ) )
			{
				bTheSame = false;
				break;
			}
		}

		if ( bTheSame && ( m_aDiscImage[ i + 7 ] & 0x7F ) == '$' )
		{
			// File already exists
			throw AsmException_FileError_FileExists( m_outputFilename );
		}
	}

	// Calculate sector address for the new file

	int sectorAddrOfThisFile;

	if ( m_aDiscImage[ 0x105 ] > 0 )
	{
		int sectorAddrOfLastFile	= m_aDiscImage[ 0x10F ] +
									  ( ( m_aDiscImage[ 0x10E ] & 0x03 ) << 8 );

		int lengthOfLastFile		= m_aDiscImage[ 0x10C ] +
									  ( m_aDiscImage[ 0x10D ] << 8 ) +
									  ( ( m_aDiscImage[ 0x10E ] & 0x30 ) << 12 );

		sectorAddrOfThisFile = sectorAddrOfLastFile + ( ( lengthOfLastFile + 0xFF ) >> 8 );
	}
	else
	{
		sectorAddrOfThisFile = 2;
	}

	int sectorLengthOfThisFile	= ( len + 0xFF ) >> 8;

	if ( sectorAddrOfThisFile + sectorLengthOfThisFile > 800 )
	{
		// Disc full
		throw AsmException_FileError_DiscFull( m_outputFilename );
	}

	// Make space in the catalog for the new file

	for ( int i = m_aDiscImage[ 0x105 ]; i > 0; i -= 8 )
	{
		for ( int j = 0; j < 8; j++ )
		{
			m_aDiscImage[ i + j + 8 ] = m_aDiscImage[ i + j ];
			m_aDiscImage[ i + j + 0x108 ] = m_aDiscImage[ i + j + 0x100 ];
		}
	}

	// Increment the file count

	m_aDiscImage[ 0x105 ] += 8;

	// Write filename

	for ( size_t j = 0; j < 7; j++ )
	{
		m_aDiscImage[ j + 8 ] = ( j < strlen( pName ) ) ? pName[ j ] : ' ';
	}

	// Write directory name

	m_aDiscImage[ 15 ] = dirName;

	// Write load address

	m_aDiscImage[ 0x108 ] = load & 0xFF;
	m_aDiscImage[ 0x109 ] = ( load & 0xFF00 ) >> 8;

	// Write exec address

	m_aDiscImage[ 0x10A ] = exec & 0xFF;
	m_aDiscImage[ 0x10B ] = ( exec & 0xFF00 ) >> 8;

	// Write length

	m_aDiscImage[ 0x10C ] = len & 0xFF;
	m_aDiscImage[ 0x10D ] = ( len & 0xFF00 ) >> 8;

	// Write sector start

	m_aDiscImage[ 0x10F ] = sectorAddrOfThisFile & 0xFF;

	// Write miscellaneous bits

	m_aDiscImage[ 0x10E ] = ( ( ( load >> 16 ) & 0x03 ) << 2 ) |
						  ( ( ( exec >> 16 ) & 0x03 ) << 6 ) |
						  ( ( ( len  >> 16 ) & 0x03 ) << 4 ) |
						  ( ( sectorAddrOfThisFile >> 8 ) & 0x03 );

	// Now write the actual file

	m_discImagePtr = sectorAddrOfThisFile * 0x100;

	Write( reinterpret_cast< const char* >( pAddr ), len );

	if ( ( m_discImagePtr & 0xFF ) != 0 )
	{
		m_aDiscImage.resize( ( m_discImagePtr & ~0xFF ) + 0x100 );
		m_discImagePtr = m_aDiscImage.size();
	}
}



/*************************************************************************************************/
/**
	DiscImage::Write()
*/
/*************************************************************************************************/
void DiscImage::Write( const char* pAddr, int len )
{
	m_aDiscImage.resize( m_discImagePtr + len );
	memcpy( &m_aDiscImage[ m_discImagePtr ], pAddr, len );
	m_discImagePtr += len;
}
