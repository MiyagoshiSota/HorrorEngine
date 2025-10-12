#pragma once
#include <filesystem>
#include <string>

class engine_string
{
public:
    // =================================================================
    // std::string (UTF-8) から std::wstring (UTF-16) へ変換
    // =================================================================
    static std::wstring to_wstring(const std::string& str)
    {
        if (str.empty())
        {
            return {};
        }

        // 必要なバッファサイズを計算
        int required_size = MultiByteToWideChar(
            CP_UTF8,      // 変換元の文字コード (UTF-8)
            0,
            str.c_str(),
            static_cast<int>(str.length()),
            nullptr,
            0
        );

        if (required_size == 0)
        {
            // エラー処理: GetLastError() などで詳細を取得可能
            return {};
        }

        std::wstring result(required_size, 0);
        MultiByteToWideChar(
            CP_UTF8,
            0,
            str.c_str(),
            static_cast<int>(str.length()),
            result.data(),   // バッファの先頭ポインタ
            required_size
        );

        return result;
    }

    // =================================================================
    // std::wstring (UTF-16) から std::string (UTF-8) へ変換
    // =================================================================
    static std::string to_string(const std::wstring& wstr)
    {
        if (wstr.empty())
        {
            return {};
        }

        // 必要なバッファサイズを計算
        int required_size = WideCharToMultiByte(
            CP_UTF8,      // 変換先の文字コード (UTF-8)
            0,
            wstr.c_str(),
            static_cast<int>(wstr.length()),
            nullptr,
            0,
            nullptr,
            nullptr
        );

        if (required_size == 0)
        {
            // エラー処理
            return {};
        }

        std::string result(required_size, 0);
        WideCharToMultiByte(
            CP_UTF8,
            0,
            wstr.c_str(),
            static_cast<int>(wstr.length()),
            result.data(),   // バッファの先頭ポインタ
            required_size,
            nullptr,
            nullptr
        );

        return result;
    }

    static std::wstring replace_extension(const std::wstring& origin, const char* ext)
    {
        std::filesystem::path p = origin.c_str();
        return p.replace_extension(ext).c_str();
    }
};
