#pragma once
#include <Windows.h>
#include <stdio.h>
#include <assert.h>
#include <strsafe.h>
#include <sal.h>

#define WMain wmain
#define ALLOC_MAX_READ (0x01F00000)
#define ALLOC_MAX_WRITE (ALLOC_MAX_READ * 2U)

INT WINAPIV WMain(_In_ INT nArgc, _In_reads_(nArgc) WCHAR *pArgv[]);
