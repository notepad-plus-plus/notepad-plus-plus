/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2013 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2017-2025 Pavel Nedev (pg.nedev@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#define sqlite3			HANDLE
#define sqlite3_stmt	HANDLE
#define SQLITE_OK		0
#define SQLITE_ROW		100


typedef const char * (*PSQLVERSION) (void);
typedef int (*PSQLOPEN16) (const void *filename, sqlite3 **ppDb);
typedef int (*PSQLPREPARE16V2) (sqlite3 *db, const void *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail);
typedef int (*PSQLSTEP) (sqlite3_stmt *pStmt);
typedef const void * (*PSQLCOLUMNTEXT16) (sqlite3_stmt *pStmt, int iCol);
typedef int (*PSQLFINALZE) (sqlite3_stmt *pStmt);
typedef int (*PSQLCLOSE) (sqlite3 *db);


extern PSQLVERSION		sqlite3_libversion;
extern PSQLOPEN16		sqlite3_open16;
extern PSQLPREPARE16V2	sqlite3_prepare16_v2;
extern PSQLSTEP			sqlite3_step;
extern PSQLCOLUMNTEXT16	sqlite3_column_text16;
extern PSQLFINALZE		sqlite3_finalize;
extern PSQLCLOSE		sqlite3_close;


bool InitSQLite();
