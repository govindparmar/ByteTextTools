#include <Windows.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <intsafe.h>
#include <strsafe.h>
#include <sal.h>

#define WMain wmain
#define ALLOC_MAX_READ (0x01F00000U)
#define ALLOC_MAX_WRITE (ALLOC_MAX_READ * 2U)
C_ASSERT(ALLOC_MAX_READ * 2U == ALLOC_MAX_WRITE);

_Ret_range_(FALSE, TRUE)
_Success_(return)

BOOL WINAPI VerifyTextBuffer(
	_In_ SIZE_T cb, 
	_In_reads_(cb) BYTE *pb
)
{
	SIZE_T i = 0;

	for (; i < cb; i++)
	{
		if (!isdigit(pb[i]))
		{
			if (!isalpha(pb[i]))
			{
				return FALSE;
			}

			if (toupper(pb[i]) < 'A' || toupper(pb[i]) > 'F')
			{
				return FALSE;
			}
		}
	}
	
	return !(i & 1);
}

_Ret_range_(FALSE, TRUE)
_Success_(return)

BOOL WINAPI HexStringToBytes(
	_In_ SIZE_T cbHexString, 
	_In_reads_(cbHexString) BYTE *bHexString,
	_In_ SIZE_T cbBinary, 
	_Out_writes_to_(cbBinary, cbBinary) BYTE *bBinary
)
{
	SIZE_T i, i2 = 0;
	UCHAR uTmp[3];
	ULONG ulTmp;
	HRESULT hr;

	uTmp[2] = '\0';
	for (i = 0; i < cbHexString - 1; i += 2)
	{
		uTmp[0] = bHexString[i];
		uTmp[1] = bHexString[i + 1];

		ulTmp = strtoul(uTmp, NULL, 16);
		hr = ULongToByte(ulTmp, &bBinary[i2]);
		if (FAILED(hr))
		{
			return FALSE;
		}

		i2++;
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
		_putws(L"Usage: T2B inputfile outputfile");
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
		dwError = ERROR_NO_MORE_ITEMS;
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
		cbOutAlloc = (cbAlloc / 2U);
		assert(cbOutAlloc <= ALLOC_MAX_WRITE);

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

		if (!VerifyTextBuffer(cbAlloc, bBuffer))
		{
			dwError = ERROR_INVALID_DATA;
			goto cleanup;
		}

		bOutBuffer = (BYTE *) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbOutAlloc);
		if (NULL == bOutBuffer)
		{
			dwError = ERROR_OUTOFMEMORY;
			goto cleanup;
		}

		if (!HexStringToBytes(cbAlloc, bBuffer, cbOutAlloc, bOutBuffer))
		{
			dwError = ERROR_NO_MORE_ITEMS;
			goto cleanup;
		}

		if (!WriteFile(hOutFile, bOutBuffer, (DWORD) cbOutAlloc, &dwWritten, NULL))
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
