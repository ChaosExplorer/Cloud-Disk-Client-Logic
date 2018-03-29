#ifndef __PROCESS_INJECT_H__
#define __PROCESS_INJECT_H__

#pragma once

void InjectProcess(HANDLE hProcess, tstring procPath, tstring procName);

void EjectProcess(HANDLE hProcess);

#endif // __PROCESS_INJECT_H__
