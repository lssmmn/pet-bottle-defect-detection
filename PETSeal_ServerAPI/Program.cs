using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using System.Linq;
using System.Text.Json;
using MySql.Data.MySqlClient;
using System.IO;

namespace PETSale_ServerAPI
{
    class Program
    {
        // --- 설정 변수 (헤더 규약 8바이트로 확장) ---
        const int CLIENT_LISTEN_PORT = 8888; // 외부 클라이언트가 접속할 포트
        // Python 서버 IP 주소를 확인하여 정확한 주소로 설정해야 합니다. (로그에 10.10.21.103이 보임)
        const string PYTHON_SERVER_IP = "10.10.21.103";
        const int PYTHON_SERVER_PORT = 5000; // Python 서버 포트

        // [수정] 헤더를 8바이트로 정의: 4B 요청 ID + 4B 데이터 크기
        const int REQUEST_ID_HEADER_SIZE = 4; // 요청 ID (키 값) 크기
        const int FILE_SIZE_HEADER_SIZE = 4;  // 이미지/JSON 데이터 크기 헤더 크기
        const int TOTAL_HEADER_SIZE = REQUEST_ID_HEADER_SIZE + FILE_SIZE_HEADER_SIZE; // 총 8 바이트

        // ✅ DB 연결 문자열 (서버 IP는 MySQL이 설치된 PC의 IP로)
        const string DB_CONN_STR =
            "Server=10.10.21.108;Port=3306;Database=PetSeal_db;Uid=root;Pwd=0000;";


        // --- 프로그램 시작 ---
        static void Main(string[] args)
        {
            Console.WriteLine($"C# TCP 중계 서버 시작. 클라이언트 대기 포트: {CLIENT_LISTEN_PORT}");

            try
            {
                TcpListener listener = new TcpListener(IPAddress.Any, CLIENT_LISTEN_PORT);
                listener.Start();
                Console.WriteLine("리스너 시작됨. 클라이언트 접속을 기다리는 중...");

                while (true)
                {
                    TcpClient client = listener.AcceptTcpClient();
                    Console.WriteLine($"\n[접속] 클라이언트 접속됨: {((IPEndPoint)client.Client.RemoteEndPoint).Address}");

                    Task.Run(() => HandleClientConnectionAsync(client));
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"서버 오류: {ex.Message}");
            }
        }


        // ----------------------------------------------------------------------
        // --- 핵심 중계 로직 함수 (8바이트 헤더 처리) ---
        // ----------------------------------------------------------------------
        static async Task HandleClientConnectionAsync(TcpClient client)
        {
            // [CS0103 해결] imageBuffer와 전체 헤더 버퍼를 최상위에서 미리 선언하여 모든 블록에서 접근 가능하게 합니다.
            byte[] imageBuffer = null;
            byte[] fullHeaderBuffer = new byte[TOTAL_HEADER_SIZE]; // 8바이트 버퍼

            using (client)
            {
                using (NetworkStream clientStream = client.GetStream())
                {
                    try
                    {
                        // 1. C++ 클라이언트로부터 전체 헤더 (요청ID + 파일크기, 총 8바이트) 수신
                        int bytesRead = await clientStream.ReadAsync(fullHeaderBuffer, 0, TOTAL_HEADER_SIZE);

                        if (bytesRead != TOTAL_HEADER_SIZE)
                        {
                            Console.WriteLine("오류: 전체 헤더 (요청ID+파일크기)를 완전히 읽지 못했습니다. 연결 종료.");
                            return;
                        }

                        // 2. 요청 ID (Key) 디코딩 (오프셋 0)
                        int requestID = BitConverter.ToInt32(fullHeaderBuffer, 0);
                        Console.WriteLine($"[C++ 수신] 요청 ID: {requestID}");

                        // 3. 파일 크기 디코딩 (오프셋 4)
                        int fileSize = BitConverter.ToInt32(fullHeaderBuffer, REQUEST_ID_HEADER_SIZE);
                        Console.WriteLine($"[C++ 수신] 파일 크기: {fileSize} 바이트");

                        // 4. 파일 데이터 수신
                        imageBuffer = new byte[fileSize];
                        int totalBytesRead = 0;

                        while (totalBytesRead < fileSize)
                        {
                            bytesRead = await clientStream.ReadAsync(imageBuffer, totalBytesRead, fileSize - totalBytesRead);
                            if (bytesRead == 0) break;
                            totalBytesRead += bytesRead;
                        }

                        if (totalBytesRead != fileSize)
                        {
                            Console.WriteLine("오류: 이미지 파일 수신이 불완전합니다. 연결 종료.");
                            return;
                        }
                        Console.WriteLine("[C++ 수신] 이미지 데이터 수신 완료.");


                        // -----------------------------------------------------------
                        // 5. Python 서버에 접속 및 데이터 전달
                        // -----------------------------------------------------------
                        using (TcpClient pythonClient = new TcpClient())
                        {
                            // PYTHON_SERVER_IP가 10.10.21.103이 맞는지 다시 확인하세요.
                            await pythonClient.ConnectAsync(PYTHON_SERVER_IP, PYTHON_SERVER_PORT);
                            Console.WriteLine($"[Python 연결] 서버({PYTHON_SERVER_IP}:{PYTHON_SERVER_PORT})에 접속 완료.");

                            using (NetworkStream pythonStream = pythonClient.GetStream())
                            {
                                // 6. Python 서버에 8바이트 헤더와 이미지 데이터 전송
                                await pythonStream.WriteAsync(fullHeaderBuffer, 0, fullHeaderBuffer.Length);
                                await pythonStream.WriteAsync(imageBuffer, 0, imageBuffer.Length);
                                Console.WriteLine("[Python 전송] 데이터 전송 완료. 결과 대기 중...");


                                // 7. Python 서버로부터 응답 헤더 수신 (요청ID + JSON 크기, 총 8바이트)
                                byte[] responseHeaderBuffer = new byte[TOTAL_HEADER_SIZE];

                                int readResultSizeHeader = await pythonStream.ReadAsync(responseHeaderBuffer, 0, TOTAL_HEADER_SIZE);

                                if (readResultSizeHeader != TOTAL_HEADER_SIZE)
                                {
                                    Console.WriteLine("오류: Python 응답 헤더를 완전히 읽지 못했습니다.");
                                    return;
                                }

                                // 8. JSON 크기 디코딩 (오프셋 4)
                                // Python 서버는 문자열 ID를 사용하더라도 이 위치에는 반드시 정확한 4B 바이너리 크기를 넣어주어야 합니다.
                                int resultSize = BitConverter.ToInt32(responseHeaderBuffer, REQUEST_ID_HEADER_SIZE);


                                // [수정] Python이 문자열 ID를 헤더에 넣어 요청 ID 불일치 오류가 나는 것을 막기 위해
                                // 응답 ID와 요청 ID를 비교하는 검증 로직을 제거했습니다.
                                Console.WriteLine($"[Python 응답] JSON 데이터 크기: {resultSize} 바이트");

                                // 9. JSON 응답 데이터 수신
                                byte[] resultBuffer = new byte[resultSize];
                                int totalResultBytesRead = 0;

                                while (totalResultBytesRead < resultSize)
                                {
                                    bytesRead = await pythonStream.ReadAsync(resultBuffer, totalResultBytesRead, resultSize - totalResultBytesRead);
                                    if (bytesRead == 0) break;
                                    totalResultBytesRead += bytesRead;
                                }

                                if (totalResultBytesRead != resultSize)
                                {
                                    Console.WriteLine("오류: Python 응답 데이터 수신이 불완전합니다.");
                                    return;
                                }

                                string resultJson = Encoding.UTF8.GetString(resultBuffer);
                                Console.WriteLine($"[Python 응답] 결과 수신 완료. 내용: {resultJson}");

                                // ✅ 추가: JSON에서 key 추출 (없으면 requestID 사용)
                                string keyForImage = requestID.ToString();
                                try
                                {
                                    using var keyDoc = JsonDocument.Parse(resultJson);
                                    if (keyDoc.RootElement.TryGetProperty("key", out var k) && k.ValueKind == JsonValueKind.String)
                                        keyForImage = k.GetString() ?? keyForImage;
                                }
                                catch { /* 파싱 실패시 requestID 사용 */ }

                                // ✅ 추가: 이미지 저장하고 DB에 넣을 경로 얻기
                                string imagePathForDb = SaveImageAndGetDbPath(keyForImage, imageBuffer);

                                // ---------------------------
                                // ✅ DB 저장 추가 부분
                                // ---------------------------
                                SaveJsonResultToDB(resultJson, requestID, imagePathForDb);

                                // ---------------------------
                                // C++ 클라이언트에게 응답 반환
                                // ---------------------------
                                await clientStream.WriteAsync(responseHeaderBuffer, 0, responseHeaderBuffer.Length);
                                await clientStream.WriteAsync(resultBuffer, 0, resultBuffer.Length);
                                Console.WriteLine("[C++ 반환] 최종 결과 전달 완료.");
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"[오류] {ex.Message}");
                    }
                    finally
                    {
                        Console.WriteLine("[종료] 클라이언트 연결 처리 완료.");
                    }
                }
            }

            // ✅ 추가: 이미지 저장 후 DB에 넣을 상대경로 반환
            static string SaveImageAndGetDbPath(string key, byte[] imageBytes)
            {
                // 실행파일 폴더 기준으로 images 폴더 생성
                string baseDir = AppContext.BaseDirectory;
                string imagesDir = Path.Combine(baseDir, "images");
                Directory.CreateDirectory(imagesDir); // 폴더 없으면 생성

                // 파일명: {key}.jpg  (필요하면 확장자 바꿔도 됨)
                string fileName = $"{key}.jpg";
                string fullPath = Path.Combine(imagesDir, fileName);

                // 실제 파일 저장
                File.WriteAllBytes(fullPath, imageBytes);

                // DB에는 상대경로 저장 (슬래시 통일)
                string relative = Path.GetRelativePath(baseDir, fullPath).Replace('\\', '/');
                return relative; // 예: "images/1991221094.jpg"
            }

            // ----------------------------------------------------------
            // ✅ Python JSON 결과를 MySQL DB에 저장 (문자열/숫자 모두 처리 가능)
            //    이미지 경로는 imagePath 매개변수로 전달받아 그대로 사용
            // ----------------------------------------------------------
            static void SaveJsonResultToDB(string json, int requestID, string imagePath)
            {
                try
                {
                    using var doc = System.Text.Json.JsonDocument.Parse(json);
                    var root = doc.RootElement;

                    string key = root.TryGetProperty("key", out var keyElem)
                        ? keyElem.GetString()
                        : requestID.ToString();

                    string result = root.TryGetProperty("result", out var resElem)
                        ? resElem.GetString()
                        : "0";

                    // 🔹 reliability: 문자열("0.575") 또는 숫자(0.575) 모두 처리
                    decimal reliability = 0.0m;
                    if (root.TryGetProperty("reliability", out var relElem))
                    {
                        if (relElem.ValueKind == JsonValueKind.Number)
                            reliability = relElem.GetDecimal();
                        else if (relElem.ValueKind == JsonValueKind.String &&
                                 decimal.TryParse(relElem.GetString(), out var parsed))
                            reliability = parsed;
                    }

                    string state = root.TryGetProperty("state", out var stateElem)
                        ? stateElem.GetString()
                        : "unknown";

                    using var conn = new MySqlConnection(DB_CONN_STR);
                    conn.Open();

                    string query = @"INSERT INTO petseal_db.petseal (KeyValue, Time, imagePath, result, reliability, state)
                         VALUES (@KeyValue, NOW(), @ImagePath, @Result, @Reliability, @State)";

                    using var cmd = new MySqlCommand(query, conn);
                    cmd.Parameters.AddWithValue("@KeyValue", key);
                    cmd.Parameters.AddWithValue("@ImagePath", imagePath);
                    cmd.Parameters.AddWithValue("@Result", result);
                    cmd.Parameters.AddWithValue("@Reliability", reliability);
                    cmd.Parameters.AddWithValue("@State", state);

                    int rows = cmd.ExecuteNonQuery();
                    Console.WriteLine($"[DB저장] ✅ 성공: {rows}행 삽입 (key={key}, img={imagePath}, result={result}, reliability={reliability}, state={state})");
                }
                catch (Exception ex)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"[DB오류] 저장 실패: {ex.Message}");
                    Console.ResetColor();
                }
            }

        }
    }
}