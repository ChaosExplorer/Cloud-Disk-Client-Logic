#ifndef __PROCESS_HOOK_H__
#define __PROCESS_HOOK_H__

#pragma once

void WINAPI HookOneProcess(DWORD dwProcId);

void WINAPI UnhookOneProcess(DWORD dwProcId);

void WINAPI HookAllProcess();

void WINAPI UnhookAllProcess();

#endif // __PROCESS_HOOK_H__
