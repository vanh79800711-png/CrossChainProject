#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <functional>
#include <cstdint>
#include "Types.hpp"

using namespace std;

// Loại mạng mà giao dịch hướng tới
enum class ChainType { EVM, UTXO };

inline string chainTypeToString(ChainType t) {
    switch (t) {
        case ChainType::EVM:  return "EVM";
        case ChainType::UTXO: return "UTXO";
    }
    return "UNKNOWN";
}

// Dữ liệu đầu vào "trung lập" mà người dùng đưa cho Factory.
// Factory sẽ dịch payload này sang cấu trúc giao dịch riêng của từng mạng.
struct TransactionPayload {
    string   sender;
    string   receiver;
    double   amount = 0.0;
    string   data;          // calldata (chỉ EVM dùng)
    uint64_t nonce  = 0;    // chỉ EVM dùng
};

// Băm giả lập (không phải mật mã thật) để sinh mã hash giao dịch
inline string makePseudoHash(const string& raw) {
    size_t h = std::hash<string>{}(raw);
    ostringstream oss;
    oss << "0x" << std::hex << std::setfill('0') << std::setw(16) << h;
    return oss.str();
}

// ======================= Product interface =======================
class ITransaction {
public:
    virtual ~ITransaction() = default;
    virtual ChainType getType() const = 0;
    virtual string    getTxHash() const = 0;
    virtual string    getSender() const = 0;
    virtual string    getReceiver() const = 0;
    virtual double    getAmount() const = 0;
    virtual string    describe() const = 0;
};

// ======================= EVM Transaction =======================
class EVMTransaction : public ITransaction {
public:
    uint64_t nonce;      // số thứ tự giao dịch của tài khoản gửi
    double   gasPrice;   // đơn vị Gwei
    uint64_t gasLimit;   // gas tối đa chấp nhận trả
    string   from;
    string   to;
    double   value;      // lượng coin chuyển
    string   data;       // calldata gọi hợp đồng

    EVMTransaction(uint64_t n, double gp, uint64_t gl,
                   string f, string t, double v, string d)
        : nonce(n), gasPrice(gp), gasLimit(gl),
          from(std::move(f)), to(std::move(t)), value(v), data(std::move(d)) {}

    ChainType getType() const override { return ChainType::EVM; }
    string getSender() const override   { return from; }
    string getReceiver() const override { return to; }
    double getAmount() const override   { return value; }

    string getTxHash() const override {
        ostringstream raw;
        raw << nonce << '|' << gasPrice << '|' << gasLimit << '|'
            << from << '|' << to << '|' << value << '|' << data;
        return makePseudoHash(raw.str());
    }

    string describe() const override {
        ostringstream o;
        o << "[EVM Tx] hash=" << getTxHash() << " nonce=" << nonce
          << " from=" << from << " to=" << to << " value=" << value
          << " gasPrice=" << gasPrice << "Gwei gasLimit=" << gasLimit
          << " data=\"" << data << "\"";
        return o.str();
    }
};

// ======================= UTXO Transaction =======================
struct UTXOInput {
    string   prevTxId;   // giao dịch trước chứa UTXO được tiêu
    uint32_t index;      // vị trí output trong giao dịch trước
    string   owner;
    double   amount;
};

struct UTXOOutput {
    string address;
    double amount;
};

class UTXOTransaction : public ITransaction {
public:
    vector<UTXOInput>  inputs;
    vector<UTXOOutput> outputs;
    vector<string>     signatures;   // mỗi input cần 1 chữ ký (giả lập)

    ChainType getType() const override { return ChainType::UTXO; }
    string getSender() const override   { return inputs.empty()  ? "" : inputs[0].owner; }
    string getReceiver() const override { return outputs.empty() ? "" : outputs[0].address; }
    double getAmount() const override   { return outputs.empty() ? 0.0 : outputs[0].amount; }

    double totalIn() const  { double s = 0; for (auto& i : inputs)  s += i.amount; return s; }
    double totalOut() const { double s = 0; for (auto& o : outputs) s += o.amount; return s; }
    double fee() const      { return totalIn() - totalOut(); }

    string getTxHash() const override {
        ostringstream raw;
        for (auto& i : inputs)  raw << i.prevTxId << ':' << i.index << ':' << i.amount << ';';
        for (auto& o : outputs) raw << o.address << ':' << o.amount << ';';
        return makePseudoHash(raw.str());
    }

    string describe() const override {
        ostringstream o;
        o << "[UTXO Tx] hash=" << getTxHash()
          << " inputs=" << inputs.size() << " outputs=" << outputs.size()
          << " signatures=" << signatures.size()
          << " (" << getSender() << " -> " << getReceiver() << ": " << getAmount()
          << ", fee=" << fee() << ")";
        return o.str();
    }
};
