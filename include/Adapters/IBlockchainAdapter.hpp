#pragma once
#include <string>
#include <vector>
#include "../Common/Types.hpp"
#include "../Common/Transaction.hpp"

using namespace std;

class IBlockchainAdapter {
public:
    virtual ~IBlockchainAdapter() = default;

    // Lấy tên định danh của mạng (ví dụ: "Ethereum", "Solana")
    virtual string getChainId() const = 0;

    // Kiểm tra số dư tài khoản
    virtual double getBalance(const string& address) = 0;

    // [BƯỚC 2 - MỚI] Gửi giao dịch đã được Factory dựng sẵn lên mạng giả lập
    virtual TransactionResult sendTransaction(const ITransaction& tx) = 0;

    // Khóa tài sản tại chuỗi nguồn (Lock Asset)
    virtual bool lockAsset(const string& sender, double amount) = 0;

    // Phát hành/cộng tài sản tại chuỗi đích (Mint Asset)
    virtual bool mintOrReleaseAsset(const string& receiver, double amount) = 0;

    // Lấy trạng thái lưu trữ của smart contract
    virtual SmartContractState getContractState(const string& contractAddress) = 0;
};
