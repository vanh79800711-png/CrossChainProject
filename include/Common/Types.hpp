#pragma once
#include <iostream>
#include <string>
#include <vector> // Mảng động
#include <unordered_map>
#include <cstdint>

using namespace std;

// 1. Cấu trúc thông điệp xuyên chuỗi (Cross-Chain Message)
struct CrossChainMessage {
    string messageId;        // Mã định danh gói tin (ví dụ: "MSG_001")
    string sourceChain;      // Mạng gửi (ví dụ: "EVM_ETH")
    string targetChain;      // Mạng nhận (ví dụ: "UTXO_BTC")
    string sender;           // Người gửi
    string receiver;         // Người nhận
    double amount;           // Lượng token chuyển
    string proof;            // Chữ ký xác thực giả định
    uint64_t timestamp;      // Thời điểm tạo
};

// 2. Kết quả thực thi giao dịch (Transaction Result)
struct TransactionResult {
    bool isSuccess;          // Thành công (true) hay Thất bại (false)
    string txHash;           // Mã hash giao dịch giả lập
    string errorMessage;     // Chi tiết lỗi nếu gặp sự cố
    double gasUsed;          // Phí gas tiêu thụ giả định
};

// 3. Trạng thái hợp đồng thông minh (Smart Contract State)
struct SmartContractState {
    string contractAddress;
    // Bảng lưu trữ trạng thái dạng Khóa -> Giá trị (Key -> Value)
    unordered_map<string, string> storage;
};