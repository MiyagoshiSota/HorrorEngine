#pragma once
#include <d3d12.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <Windows.h>
#include <comdef.h>

/// <summary>
/// HRESULTが失敗した場合に例外をスローするヘルパー関数
/// エラー発生時のファイル名、行数、エラーメッセージを確実に捕捉します
/// </summary>
/// <param name="hr">チェックするHRESULT</param>
/// <exception cref="std::runtime_error">HRESULTが失敗した場合</exception>
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        _com_error err(hr);
        std::string errorMsg = "HRESULT failed: 0x";
        errorMsg += std::to_string(static_cast<unsigned int>(hr));
        errorMsg += " - ";
        
        // TCHAR* を char* に変換
        const TCHAR* tcharMsg = err.ErrorMessage();
#ifdef UNICODE
        // Unicode ビルドの場合、WideCharToMultiByte を使用
        int size = WideCharToMultiByte(CP_ACP, 0, tcharMsg, -1, nullptr, 0, nullptr, nullptr);
        if (size > 0)
        {
            std::vector<char> buffer(size);
            WideCharToMultiByte(CP_ACP, 0, tcharMsg, -1, buffer.data(), size, nullptr, nullptr);
            errorMsg += buffer.data();
        }
#else
        // ANSI ビルドの場合、そのまま使用
        errorMsg += tcharMsg;
#endif
        throw std::runtime_error(errorMsg);
    }
}
