// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "C:\Program Files\Cheat Engine\plugins\cepluginsdk.h"
#include "C:\Program Files\Cheat Engine\plugins\lua.hpp"
#include <string>
#include <vector>
#include <bitset>

static const char version[] = "BL4 Serial Decoder v0.1";


unsigned char reverseBits(unsigned char b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}
std::vector<unsigned char> decodeBase85(const std::string& input) {
    std::string alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+-;<=>?@^_`{/}~";
    int lookup[256];
    std::fill(std::begin(lookup), std::end(lookup), -1);
    for (int i = 0; i < 85; ++i)
        lookup[(unsigned char)alphabet[i]] = i;

    std::vector<unsigned char> output;
    size_t len = input.size();
    size_t i = 0;

    int remaining = len % 5;
    size_t fullGroups = len / 5;

    for (size_t g = 0; g < fullGroups; ++g) {
        uint32_t value = 0;
        for (int j = 0; j < 5; ++j)
            value = value * 85 + lookup[(unsigned char)input[g * 5 + j]];
        for (int b = 3; b >= 0; --b)
            output.push_back((value >> (8 * b)) & 0xFF);
    }
    if (remaining > 0) {
        uint32_t value = 0;
        for (int j = 0; j < remaining; ++j)
            value = value * 85 + lookup[(unsigned char)input[fullGroups * 5 + j]];
        int padIndex = lookup[(unsigned char)'~'];
        for (int j = remaining; j < 5; ++j)
            value = value * 85 + padIndex;
        int byteCount = remaining - 1;
        for (int b = 3; b >= 4 - byteCount; --b)
            output.push_back(static_cast<unsigned char>((value >> (8 * b)) & 0xFF));
    }

    return output;
}

unsigned int decode_varbit(std::string& remaining_string) {

    std::string len_string = remaining_string.substr(0, 5);
    std::reverse(len_string.begin(), len_string.end());
    const unsigned int len = std::bitset<5>(len_string).to_ulong();
    remaining_string = remaining_string.substr(5);
    std::string data_string = remaining_string.substr(0, len);
    std::reverse(data_string.begin(), data_string.end());
    remaining_string = remaining_string.substr(len);
    return std::bitset<32>(data_string).to_ulong();
}

unsigned int decode_varint(std::string& remaining_string) {
    bool cont = true;
    std::string data_string;
    while (cont) {
        std::string block_data = remaining_string.substr(0, 4);
        std::reverse(data_string.begin(), data_string.end());
        data_string += block_data;
        std::reverse(data_string.begin(), data_string.end());
        if (remaining_string.at(4) == '0') {
            cont = false;
        }
        remaining_string = remaining_string.substr(5);
    }
    return std::bitset<16>(data_string).to_ulong();
}

std::string decode_part(std::string& remaining_string) {
    int first_value = decode_varint(remaining_string);
    bool flag = remaining_string.at(0) == '1';
    remaining_string = remaining_string.substr(1);
    if (flag) {
        int second_value = decode_varint(remaining_string);
        remaining_string = remaining_string.substr(3);
        return "{" + std::to_string(first_value) + ":" + std::to_string(second_value) + "}";
    }
    else {
        std::string subtype = remaining_string.substr(0, 2);
        remaining_string = remaining_string.substr(2);
        if (subtype == "10") {
            return "{" + std::to_string(first_value) + "}";
        }
        else if (subtype == "01") {
            std::string list_string = "[";
            bool first_in_list = true;
            while (true) {
                const char bitchar = remaining_string.at(0);
                if (bitchar == '0') {
                    if (remaining_string.at(1) == '0') {
                        remaining_string = remaining_string.substr(2);
                        break;
                    }
                    else {
                        remaining_string = remaining_string.substr(2);
                    }
                }
                else {
                    if (!first_in_list) list_string += " ";
                    std::string subtype = remaining_string.substr(1, 2);
                    if (subtype == "00") {
                        remaining_string = remaining_string.substr(3);
                        list_string += std::to_string(decode_varint(remaining_string));
                    }
                    else if (subtype == "10") {
                        remaining_string = remaining_string.substr(3);
                        list_string += std::to_string(decode_varbit(remaining_string));
                    }
                    if (first_in_list) first_in_list = false;
                    else if (subtype == "11") {
                        return "part_decode_error";
                    }
                }
            }
            list_string += "]";
            return "{" + std::to_string(first_value) + ":" + list_string + "}";
        }
        else {
            return "part_decode_error";
        }
    }
}

std::string decode_bitstring(std::string& bitstring) {
    bool first_value = true;
    std::string output_string = "";
    bitstring = bitstring.substr(7);
    int trailing_separators = 0;
    std::string separator_0 = "|";
    std::string separator_1 = ",";
    while (bitstring.size() > 1) {
        const char bitchar = bitstring.at(0);
        if (bitchar == '0') {
            const char separator_type = bitstring.at(1);
            if (separator_type == '0') output_string += separator_0;
            else output_string += separator_1;
            trailing_separators++;
            bitstring = bitstring.substr(2);
        }
        else {
            trailing_separators = 0;
            if (!first_value) output_string += " ";
            std::string value_type = bitstring.substr(1, 2);
            if (value_type == "00") {
                bitstring = bitstring.substr(3);
                unsigned int value = decode_varint(bitstring);
                output_string += std::to_string(value);
            }
            else if (value_type == "10") {
                bitstring = bitstring.substr(3);
                unsigned int value = decode_varbit(bitstring);
                output_string += std::to_string(value);
            }
            else if (value_type == "01") {
                bitstring = bitstring.substr(3);
                std::string value = decode_part(bitstring);
                if (value == "part_decode_error") {
                    output_string += " " + value;
                    return output_string;
                }
                else {
                    output_string += value;
                }
            }
            else if (value_type == "11") {
                return output_string;
            }
            else {
                bitstring = bitstring.substr(1);
            }
            if (first_value) first_value = false;
        }
    }
    if (trailing_separators > 1) {
        output_string = output_string.substr(0, output_string.size() - (trailing_separators - 1));
    }
    return output_string;
}

int bl4_decode(lua_State* L) {
    size_t len;
    std::string input = luaL_checklstring(L, 1, &len);
    if (input.size() < 2 || input.substr(0, 2) != "@U") {
        std::string error_str = "Invalid serial: " + input;
        lua_pushlstring(L, error_str.c_str(), error_str.size());
    }
    std::string data = input.substr(2);
    std::vector<unsigned char> decoded_b85 = decodeBase85(data);
    for (auto& b : decoded_b85) {
        b = reverseBits(b);
    }
    std::string bitString;
    bitString.reserve(decoded_b85.size() * 8);

    for (unsigned char byte : decoded_b85) {
        for (int i = 7; i >= 0; --i)
            bitString.push_back((byte & (1 << i)) ? '1' : '0');
    }
    std::string decoded = decode_bitstring(bitString);
    lua_pushlstring(L, decoded.c_str(), decoded.size());
    return 1; // returning 1 value to Lua
}

BOOL __stdcall CEPlugin_GetVersion(PPluginVersion pv, int sizeofpluginversion){
    if (pv) {
        pv->version = 1;
        pv->pluginname = (char*)version;
    }
    return TRUE;
}
BOOL __stdcall CEPlugin_InitializePlugin(PExportedFunctions exportedFunctions, int pluginid) {
    // Access CE’s Lua state
    lua_State* L = exportedFunctions->GetLuaState();

    // Register your function in CE’s Lua environment
    lua_register(L, "bl4_decode", bl4_decode);

    return TRUE;
}

BOOL __stdcall CEPlugin_DisablePlugin() {
    return TRUE;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

