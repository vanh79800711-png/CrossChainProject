#include <iostream>
#include "include/Common/Types.hpp"
#include "include/Adapters/IBlockchainAdapter.hpp"

using namespace std;

int main() {
    // Khởi tạo thử một gói tin chuyển tiền xuyên chuỗi
    CrossChainMessage msg;
    msg.messageId = "TX_001";
    msg.sourceChain = "Ethereum";
    msg.targetChain = "Solana";
    msg.sender = "Alice";
    msg.receiver = "Bob";
    msg.amount = 25.5;

    cout << "=======================================\n";
    cout << "KIEM TRA HOAN TAT BUOC 1 THANH CONG!\n";
    cout << "Thong diep: " << msg.sender << " chuyen " << msg.amount 
         << " coin tu " << msg.sourceChain << " toi " << msg.targetChain << "\n";
    cout << "=======================================\n";

    return 0;
}