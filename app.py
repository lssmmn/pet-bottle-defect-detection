import socket
import struct
from ultralytics import YOLO
import cv2
import numpy as np
import json  # ← 추가

# 모델 로드
model = YOLO('best.pt')

# 서버 설정
HOST = '0.0.0.0'
PORT = 5000

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((HOST, PORT))
server.listen(5)
print("Server ready, waiting for connection...")
print("=============================")
print("\n\n")


while True:
    client, addr = server.accept()
    print(f"Connected by {addr}")
 
    # 먼저 요청 ID(4바이트) + 이미지 길이(4바이트) 받기
    header = client.recv(8)
    if not header or len(header) < 8:
        client.close()
        continue



    # Little-Endian 방식으로 해석
    request_id, img_size = struct.unpack('<II', header)
    print(f"Request ID: {request_id}")
    print(f"Expecting image size: {img_size} bytes")



    # 이미지 데이터 받기
    img_bytes = b''
    while len(img_bytes) < img_size:
        packet = client.recv(img_size - len(img_bytes))
        if not packet:
            break
        img_bytes += packet



    # 이미지 디코딩
    nparr = np.frombuffer(img_bytes, np.uint8)
    img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)



    # YOLO 예측
    # 느슨하게 (기본 0.25 -> 0.20)
    results = model.predict(img, conf=0.19,  verbose=False)
    result = results[0]



    print("\n=== YOLO Detection Detail ===")
    # box.cls : 클래스 ID
    # box.conf : confidence (신뢰도, 0~1 사이)
    # box.xyxy : 바운딩 박스 좌표
    # box.xywh : 바운딩 박스를 중심 좌표와 너비/높이로 표현
    #  box.keypoints : (optional) keypoint 정보 
    if result.boxes is not None and len(result.boxes) > 0:
        for i, box in enumerate(result.boxes):
            cls_id = int(box.cls[0])
            conf = float(box.conf[0])
            xyxy = box.xyxy[0].tolist()
            class_name = model.names[cls_id]
            print(f"[{i}] Class={class_name} (ID={cls_id}), Conf={conf:.3f}, BBox={xyxy}")
    else:
        print("No detections")
    print("=============================")


    # 검출된 클래스와 신뢰도 추출
    if result.boxes is not None and len(result.boxes) > 0:
        # 가장 높은 confidence 기준으로 하나만 전송
        best_box = max(result.boxes, key=lambda b: float(b.conf[0]))
        class_id = int(best_box.cls[0])
        reliability = float(best_box.conf[0])
        result_value = str(class_id)
    else:
        # 검출 없음
        result_value = ""
        reliability = 0.0

    # JSON 데이터 생성
    verdict_json = json.dumps({
        "key": str(request_id),   # ← 요청 ID 추가
        "state": "normal",
        "result": result_value,
        "reliability": f"{reliability:.3f}"
    }).encode('utf-8')



    # 요청 ID(4바이트) + JSON 길이(4바이트) + JSON 본문 전송
    verdict_header = struct.pack('<II', request_id, len(verdict_json))
    client.send(verdict_header + verdict_json)

    # 결과 전송
    client.close()

    # === 결과 ===
    print(f"\n 결과 전송 완료: {verdict_json.decode('utf-8')}")
    print("\n [결과 요약]")
    if result.boxes is not None and len(result.boxes) > 0:
        print(f"요청 키값 : {request_id}")
        print(f"클래스 ID : {class_id}")
        print(f"클래스명   : {model.names[class_id]}")
        print(f"신뢰도     : {reliability:.3f}")
    else:
        print("검출된 객체 없음")

    print("=============================")
    print("=============================")
    print("\n")