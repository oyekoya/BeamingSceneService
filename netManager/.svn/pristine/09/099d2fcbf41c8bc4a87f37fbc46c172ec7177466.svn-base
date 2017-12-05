/// \file
/// \brief Contains WebClientList, a client for communicating with a HTTP list of clients
///
/// This file is part of RakNet Copyright 2008 Kevin Jenkins.
///
/// Usage of RakNet is subject to the appropriate license agreement.
/// Creative Commons Licensees are subject to the
/// license found at
/// http://creativecommons.org/licenses/by-nc/2.5/
/// Single application licensees are subject to the license found at
/// http://www.jenkinssoftware.com/SingleApplicationLicense.html
/// Custom license users are subject to the terms therein.
/// GPL license users are subject to the GNU General Public
/// License as published by the Free
/// Software Foundation

#include "PHPConnections.h"
#include "HTTPConnection.h"
#include "RakSleep.h"
#include "RakString.h"
#include "RakNetTypes.h"
#include "GetTime.h"
#include "RakAssert.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "Itoa.h"



// Column with this header contains the name of the client, passed to UploadTable()
static const char *CLIENT_NAME_COMMAND="__CLIENT_NAME";
// Column with this header contains the port of the client, passed to UploadTable()
static const char *CLIENT_PORT_COMMAND="__CLIENT_PORT";
// Column with this header contains the IP address of the client, passed to UploadTable()
static const char *SYSTEM_ADDRESS_COMMAND="_System_Address";
// Returned from the PHP server indicating when this row was last updated.
static const char *LAST_UPDATE_COMMAND="__SEC_AFTER_EPOCH_SINCE_LAST_UPDATE";

using namespace RakNet;
using namespace DataStructures;

PHPConnections::PHPConnections()
    : nextRepost(0)
{
    Map<RakString, RakString>::IMPLEMENT_DEFAULT_COMPARISON();
}
PHPConnections::~PHPConnections()
{
}
void PHPConnections::Init(HTTPConnection *_http, const char *_path)
{
	http=_http;
	pathToPHP=_path;
}

void PHPConnections::SetField( RakNet::RakString columnName, RakNet::RakString value )
{
	if (columnName.IsEmpty())
		return;

	if (columnName==CLIENT_NAME_COMMAND ||
		columnName==CLIENT_PORT_COMMAND ||
		columnName==LAST_UPDATE_COMMAND)
	{
		RakAssert("PHPConnections::SetField attempted to set reserved column name" && 0);
		return;
	}

    fields.Set(columnName, value);
}
unsigned int PHPConnections::GetFieldCount(void) const
{
	return fields.Size();
}
void PHPConnections::GetField(unsigned int index, RakNet::RakString &columnName, RakNet::RakString &value)
{
	RakAssert(index < fields.Size());
	columnName=fields.GetKeyAtIndex(index);
	value=fields[index];
}
void PHPConnections::SetFields(DataStructures::Table *table)
{
	ClearFields();

	unsigned columnIndex, rowIndex;
	DataStructures::Table::Row *row;
	
	for (rowIndex=0; rowIndex < table->GetRowCount(); rowIndex++)
	{
		row = table->GetRowByIndex(rowIndex, 0);
		for (columnIndex=0; columnIndex < table->GetColumnCount(); columnIndex++)
		{
			SetField( table->ColumnName(columnIndex), row->cells[columnIndex]->ToString(table->GetColumnType(columnIndex)) );
		}
	}
}

void PHPConnections::ClearFields(void)
{
	fields.Clear();
	nextRepost=0;
}

void PHPConnections::UploadTable(RakNet::RakString uploadPassword, RakNet::RakString clientName, unsigned short clientPort, bool autoRepost)
{
	clientNameParam=clientName;
	clientPortParam=clientPort;
	currentOperation="";
	currentOperation="?query=upload&uploadPassword=";
	currentOperation+=uploadPassword;
	SendOperation();

	if (autoRepost)
		nextRepost=RakNet::GetTimeMS()+50000;
	else
		nextRepost=0;
}
void PHPConnections::DownloadTable(RakNet::RakString downloadPassword)
{
	currentOperation="?query=download&downloadPassword=";
	currentOperation+=downloadPassword;
	SendOperation();
}
void PHPConnections::UploadAndDownloadTable(RakNet::RakString uploadPassword, RakNet::RakString downloadPassword, RakNet::RakString clientName, unsigned short clientPort, bool autoRepost)
{
	clientNameParam=clientName;
	clientPortParam=clientPort;
	currentOperation="?query=upDown&downloadPassword=";
	currentOperation+=downloadPassword;
	currentOperation+="&uploadPassword=";
	currentOperation+=uploadPassword;

	SendOperation();

	if (autoRepost)
		nextRepost=RakNet::GetTimeMS()+50000;
	else
		nextRepost=0;
}

HTTPReadResult PHPConnections::ProcessHTTPRead(RakNet::RakString httpRead)
{
	const char *c = (const char*) httpRead.C_String(); // current position
	HTTPReadResult resultCode=HTTP_RESULT_EMPTY;

	lastDownloadedTable.Clear();


	if (*c=='\n')
		c++;
	char buff[256];
	int buffIndex;
	bool isCommand=true;
	DataStructures::List<RakNet::RakString> columns;
	DataStructures::List<RakNet::RakString> values;
	RakNet::RakString curString;
	bool isComment=false;
	buffIndex=0;
	while(c && *c)
	{
		// 3 is comment
		if (*c=='\003')
		{
			isComment=!isComment;
			c++;
			continue;
		}
		if (isComment)
		{
			c++;
			continue;
		}

		// 1 or 2 separates fields
		// 4 separates rows
		if (*c=='\001')
		{
			if (isCommand)
			{
				buff[buffIndex]=0;
				columns.Push(RakString::NonVariadic(buff), __FILE__, __LINE__);
				isCommand=false;
				if (buff[0]!=0)
					resultCode=HTTP_RESULT_GOT_TABLE;
			}
			else
			{
				buff[buffIndex]=0;
				values.Push(RakString::NonVariadic(buff), __FILE__, __LINE__);
				isCommand=true;
			}
			buffIndex=0;
		}
		else if (*c=='\002')
		{
			buff[buffIndex]=0;
			buffIndex=0;
			values.Push(RakString::NonVariadic(buff), __FILE__, __LINE__);
			isCommand=true;
			PushColumnsAndValues(columns, values);
			columns.Clear(true, __FILE__, __LINE__);
			values.Clear(true, __FILE__, __LINE__);

		}
		else
		{
			if (buffIndex<256-1)
				buff[buffIndex++]=*c;
		}
		c++;
	}
	if (buff[0] && columns.Size()==values.Size()+1)
	{
		buff[buffIndex]=0;
		values.Push(RakString::NonVariadic(buff), __FILE__, __LINE__);
	}

	PushColumnsAndValues(columns, values);

	return resultCode;
}
void PHPConnections::PushColumnsAndValues(DataStructures::List<RakNet::RakString> &columns, DataStructures::List<RakNet::RakString> &values)
{
	DataStructures::Table::Row *row=0;

	unsigned int i;
	for (i=0; i < columns.Size() && i < values.Size(); i++)
	{
		if (columns[i].IsEmpty()==false)
		{
			unsigned col = lastDownloadedTable.ColumnIndex(columns[i]);
			if(col == (unsigned)-1)
			{
				col = lastDownloadedTable.AddColumn(columns[i], DataStructures::Table::STRING);
			}

			if (row==0)
			{
				row = lastDownloadedTable.AddRow(lastDownloadedTable.GetAvailableRowId());
			}
			row->UpdateCell(col,values[i].C_String());
		}
	}
}
const DataStructures::Table *PHPConnections::GetLastDownloadedTable(void) const
{
	return &lastDownloadedTable;
}
void PHPConnections::SendOperation(void)
{
	RakString outgoingMessageBody;
	char buff[64];

	outgoingMessageBody += CLIENT_PORT_COMMAND;
	outgoingMessageBody += '\001';
	outgoingMessageBody += Itoa(clientPortParam,buff,10);
	outgoingMessageBody += '\001';
	outgoingMessageBody += CLIENT_NAME_COMMAND;
	outgoingMessageBody += '\001';
	outgoingMessageBody += clientNameParam;

	for (unsigned i = 0; i < fields.Size(); i++)
	{
		RakString value = fields[i];
		value.URLEncode();
		outgoingMessageBody += RakString("\001%s\001%s",
			fields.GetKeyAtIndex(i).C_String(),
			value.C_String());
	}

	RakString postURL;
	postURL+=pathToPHP;
	postURL+=currentOperation;
	http->Post(postURL.C_String(), outgoingMessageBody, "application/x-www-form-urlencoded");

}
void PHPConnections::Update(void)
{
	if (http->IsBusy())
		return;


	if (nextRepost==0 || fields.Size()==0)
		return;

	RakNetTimeMS time = GetTimeMS();

	// Entry deletes itself after 60 seconds, so keep reposting if set to do so
	if (time > nextRepost)
	{
		nextRepost=RakNet::GetTimeMS()+50000;
		SendOperation();
	}
}
