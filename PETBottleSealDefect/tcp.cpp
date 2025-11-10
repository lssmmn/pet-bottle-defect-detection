// tcp.cpp
#include "pch.h"
#include <opencv2/opencv.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <random>
#pragma comment(lib, "ws2_32.lib")

bool SendData(SOCKET sock, const char* data, int len)
{
    int totalSent = 0;
    while (totalSent < len)
    {
        int sent = send(sock, data + totalSent, len - totalSent, 0);
        if (sent == SOCKET_ERROR) return false;
        totalSent += sent;
    }
    return true;
}

bool ReceiveData(SOCKET sock, char* buffer, int len)
{
    int totalRecv = 0;
    while (totalRecv < len)
    {
        int recv_count = recv(sock, buffer + totalRecv, len - totalRecv, 0);
        if (recv_count <= 0) return false;
        totalRecv += recv_count;
    }
    return true;
}

char* IntToBytes(int value)
{
    char* bytes = new char[4];
    bytes[0] = (char)(value & 0xFF);
    bytes[1] = (char)((value >> 8) & 0xFF);
    bytes[2] = (char)((value >> 16) & 0xFF);
    bytes[3] = (char)((value >> 24) & 0xFF);
    return bytes;
}

int GenerateRequestID()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dis(1, INT_MAX);
    return dis(gen);
}

bool SendImageToTcpServer(const cv::Mat& image, CString& result)
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* addrResult = NULL, * ptr = NULL, hints;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return false;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo("10.10.21.108", "8888", &hints, &addrResult) != 0)
    {
        WSACleanup();
        return false;
    }

    for (ptr = addrResult; ptr != NULL; ptr = ptr->ai_next)
    {
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET)
        {
            WSACleanup();
            return false;
        }

        if (connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
        {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(addrResult);

    if (ConnectSocket == INVALID_SOCKET)
    {
        WSACleanup();
        return false;
    }

    // JPG ÀÎÄÚµù (¾ÐÃà ¾øÀÌ ±âº» Ç°Áú)
    std::vector<uchar> buffer;
    cv::imencode(".jpg", image, buffer);
    int imageSize = buffer.size();

    int requestID = GenerateRequestID();
    char* requestIDBytes = IntToBytes(requestID);
    char* imageSizeBytes = IntToBytes(imageSize);

    char header[8];
    memcpy(header, requestIDBytes, 4);
    memcpy(header + 4, imageSizeBytes, 4);

    delete[] requestIDBytes;
    delete[] imageSizeBytes;

    if (!SendData(ConnectSocket, header, 8))
    {
        closesocket(ConnectSocket);
        WSACleanup();
        return false;
    }

    if (!SendData(ConnectSocket, (char*)buffer.data(), imageSize))
    {
        closesocket(ConnectSocket);
        WSACleanup();
        return false;
    }

    int timeout = 30000;
    setsockopt(ConnectSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    char responseHeader[8];
    if (!ReceiveData(ConnectSocket, responseHeader, 8))
    {
        closesocket(ConnectSocket);
        WSACleanup();
        return false;
    }

    int resultSize = *(reinterpret_cast<int*>(responseHeader + 4));

    if (resultSize > 0 && resultSize < 10000)
    {
        std::vector<char> resultBuffer(resultSize + 1);
        if (ReceiveData(ConnectSocket, resultBuffer.data(), resultSize))
        {
            resultBuffer[resultSize] = '\0';
            CString jsonResponse = CString(resultBuffer.data());

            int dataPos = jsonResponse.Find(_T("\"result\""));
            if (dataPos != -1)
            {
                int colonPos = jsonResponse.Find(_T(":"), dataPos);
                if (colonPos != -1)
                {
                    int quoteStart = jsonResponse.Find(_T("\""), colonPos);
                    int quoteEnd = jsonResponse.Find(_T("\""), quoteStart + 1);

                    if (quoteStart != -1 && quoteEnd != -1)
                    {
                        CString dataValue = jsonResponse.Mid(quoteStart + 1, quoteEnd - quoteStart - 1);
                        dataValue.Trim();

                        if (dataValue == _T("0"))
                        {
                            result = _T("¶Ñ²± ºÎ·¯Áü|Broken Cap");
                        }
                        else if (dataValue == _T("1"))
                        {
                            result = _T("¸µ ºÎ·¯Áü|Broken Ring");
                        }
                        else if (dataValue == _T("2"))
                        {
                            result = _T("Á¤»ó|Good Cap");
                        }
                        else if (dataValue == _T("3"))
                        {
                            result = _T("Ä¸ ´À½¼ÇÔ|Loose Cap");
                        }
                        else if (dataValue == _T("4"))
                        {
                            result = _T("Ä¸ ¾øÀ½|No Cap");
                        }
                        else
                        {
                            result = _T("°´Ã¼ ¾øÀ½|Unknown");
                        }

                        int confPos = jsonResponse.Find(_T("\"reliability\""));
                        if (confPos != -1)
                        {
                            int confColonPos = jsonResponse.Find(_T(":"), confPos);
                            if (confColonPos != -1)
                            {
                                CString confPart = jsonResponse.Mid(confColonPos + 1);
                                confPart.Trim();

                                int endPos = confPart.FindOneOf(_T(",}"));
                                if (endPos != -1)
                                {
                                    CString confValue = confPart.Left(endPos);
                                    confValue.Trim();
                                    confValue.Remove(_T('\"'));

                                    result = result + _T("|") + confValue;
                                }
                            }
                        }
                    }
                    else
                    {
                        result = _T("ÆÄ½Ì ½ÇÆÐ|Parse Error");
                    }
                }
                else
                {
                    result = _T("ÆÄ½Ì ½ÇÆÐ|Parse Error");
                }
            }
            else
            {
                result = _T("°´Ã¼ ¾øÀ½|No Data");
            }
        }
        else
        {
            closesocket(ConnectSocket);
            WSACleanup();
            return false;
        }
    }

    closesocket(ConnectSocket);
    WSACleanup();
    return true;
}