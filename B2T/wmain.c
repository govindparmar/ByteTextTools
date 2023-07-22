#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <strsafe.h>
#include <sal.h>

#define WMain wmain
#define ALLOC_MAX_READ (0x01F00000U)
#define ALLOC_MAX_WRITE (ALLOC_MAX_READ * 2U)

_Ret_range_(FALSE, TRUE)
_Success_(return)

BOOL WINAPI BytesToHexString(
	_In_ SIZE_T cbBytes,
	_In_reads_(cbBytes) BYTE *pbBytes,
	_In_ SIZE_T cbText,
	_Out_writes_to_(cbText, cbText) BYTE *pbText
)
{
	SIZE_T i = 0, i2 = 0;

	for (i = 0; i < cbBytes; i++)
	{
		if (i2 + 2 > cbText)
		{
			return FALSE;
		}
		StringCchPrintfA(pbText + i2, cbText - i2, "%.2hhX", pbBytes[i]);
		i2 += 2;
	}
	return TRUE;
}

INT WINAPIV WMain(_In_ INT nArgc, _In_reads_(nArgc) WCHAR *pArgv[])
{
	CONST HANDLE hHeap = GetProcessHeap();
	HANDLE hFile = INVALID_HANDLE_VALUE, hOutFile = INVALID_HANDLE_VALUE;
	LARGE_INTEGER liSize;
	SIZE_T cbAlloc, cbOutAlloc;
	BYTE *bBuffer = NULL, *bOutBuffer = NULL;
	DWORD dwError, dwRead, dwWritten;

	if (nArgc < 3)
	{
		_putws(L"Usage: B2T inputfile outputfile");
		return 0;
	}

	hFile = CreateFileW(pArgv[1], GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		dwError = GetLastError();
		goto cleanup;
	}
	GetFileSizeEx(hFile, &liSize);
	if (0I64 == liSize.QuadPart)
	{
		dwError = ERROR_INCORRECT_SIZE;
		goto cleanup;
	}
	hOutFile = CreateFileW(pArgv[2], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hOutFile)
	{
		dwError = GetLastError();
		goto cleanup;
	}

	do
	{
		cbAlloc = min(liSize.QuadPart, ALLOC_MAX_READ);
		cbOutAlloc = cbAlloc * 2U;
		assert(cbOutAlloc <= ALLOC_MAX_WRITE);
		cbOutAlloc++;

		bBuffer = (BYTE *) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbAlloc);
		if (NULL == bBuffer)
		{
			dwError = ERROR_OUTOFMEMORY;
			goto cleanup;
		}

		if (!ReadFile(hFile, bBuffer, (DWORD) cbAlloc, &dwRead, NULL))
		{
			dwError = GetLastError();
			goto cleanup;
		}

		bOutBuffer = (BYTE *) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbOutAlloc);
		if (NULL == bOutBuffer)
		{
			dwError = ERROR_OUTOFMEMORY;
			goto cleanup;
		}

		if (!BytesToHexString(cbAlloc, bBuffer, cbOutAlloc, bOutBuffer))
		{
			dwError = ERROR_INSUFFICIENT_BUFFER;
			goto cleanup;   
		}

		if (!WriteFile(hOutFile, bOutBuffer, (DWORD)(cbOutAlloc - 1), &dwWritten, NULL))
		{
			dwError = GetLastError();
			goto cleanup;
		}

		HeapFree(hHeap, 0, bOutBuffer);
		bOutBuffer = NULL;
		HeapFree(hHeap, 0, bBuffer);
		bBuffer = NULL;

		liSize.QuadPart -= cbAlloc;
	}
	while(liSize.QuadPart);
	dwError = ERROR_SUCCESS;
cleanup:
	if (bOutBuffer != NULL)
	{
		HeapFree(hHeap, 0, bOutBuffer);
		bOutBuffer = NULL;
	}
	if (bBuffer != NULL)
	{
		HeapFree(hHeap, 0, bBuffer);
		bBuffer = NULL;
	}
	if (hOutFile != INVALID_HANDLE_VALUE)
	{
		FlushFileBuffers(hOutFile);
		CloseHandle(hOutFile);
		hOutFile = INVALID_HANDLE_VALUE;
	}
	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
	return (INT) dwError;
}
