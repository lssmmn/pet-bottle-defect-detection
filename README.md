# YOLO Object Detection TCP Server

이 프로젝트는 **Python + YOLOv8** 기반의 **객체 검출(Object Detection) TCP 서버**입니다.  
클라이언트로부터 이미지를 수신한 뒤 YOLO 모델로 분석하고,  
검출된 결과(클래스 ID, 신뢰도 등)를 JSON 형태로 응답합니다.

---

## 주요 기능

- TCP 통신 기반 서버 구현 
- YOLOv8 모델을 이용한 실시간 객체 검출  
- 요청 ID와 JSON 기반 결과 응답  
- 가장 높은 Confidence(신뢰도)의 객체만 전송  

---

## 클라이언트 → 서버 전송 구조

| 항목         | 크기                           | 설명                         |
| ---------- | ---------------------------- | -------------------------- |
| Request ID | 4바이트 (uint32, Little-Endian) | 요청 식별자                     |
| Image Size | 4바이트 (uint32, Little-Endian) | 전송할 이미지의 크기                |
| Image Data | 가변                           | JPEG 또는 PNG 등 인코딩된 이미지 바이트 |

---

## 서버 → 클라이언트 응답 구조

| 항목         | 크기                           | 설명                 |
| ---------- | ---------------------------- | ------------------ |
| Request ID | 4바이트 (uint32, Little-Endian) | 요청 식별자             |
| JSON Size  | 4바이트 (uint32, Little-Endian) | JSON 본문의 길이        |
| JSON Body  | 가변                           | 결과 데이터 (UTF-8 인코딩) |
