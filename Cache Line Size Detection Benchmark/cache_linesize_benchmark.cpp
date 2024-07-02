#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

using namespace std;

constexpr int iterations{10000000}; // the benchmark time tuning

// Template to create a struct with various alignments
//Align both the struct and its members to ensure it fits on the same cache line
template <std::size_t Align>
struct alignas(Align) CacheLineShared {
    alignas(Align/2) std::atomic_uint64_t object1{};
    alignas(Align/2) std::atomic_uint64_t object2{}; 
};

// Function to measure time
inline auto now() noexcept { 
    return std::chrono::high_resolution_clock::now(); 
}

// Function to induce false sharing
template <size_t Align, bool which>
void false_sharing(CacheLineShared<Align>& data) {
    const auto start_time{now()};

    for (uint64_t count{}; count != iterations; count++) {
        //using atomic operations to disregard memory order ensuring false sharing occurs
        if constexpr (which)
            data.object1.fetch_add(1, std::memory_order_relaxed);
        else
            data.object2.fetch_add(1, std::memory_order_relaxed);
    }

    const auto end_time = now();
    std::chrono::duration<double, std::milli> total_time = end_time - start_time;

    //save time measurement 
    if constexpr (which)
        data.object1 = total_time.count();
    else
        data.object2 = total_time.count();
}

int main() {

    //specify all possible cache line size
    constexpr size_t alignments[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    cout << "stride, milliseconds" << endl;
    //repeat measurement 10 times to ensure accuracy
    for (int i{0}; i < 10; i++){
        for (auto align : alignments) {
            uint64_t time_taken = 0;

            //lambda function to create structs with various alignments using templaet
            auto run_test = [&]<size_t A>() {
                CacheLineShared<A> shared_data;
                std::thread t1(false_sharing<A, 0>, std::ref(shared_data));
                std::thread t2(false_sharing<A, 1>, std::ref(shared_data));
                t1.join();
                t2.join();

                //record time taken
                time_taken = shared_data.object1 + shared_data.object2;
            };

                switch (align) {
                    case 16: run_test.operator()<16>(); break;
                    case 32: run_test.operator()<32>(); break;
                    case 64: run_test.operator()<64>(); break;
                    case 128: run_test.operator()<128>(); break;
                    case 256: run_test.operator()<256>(); break;
                    case 512: run_test.operator()<512>(); break;
                    case 1024: run_test.operator()<1024>(); break;
                    case 2048: run_test.operator()<2048>(); break;
                    case 4096: run_test.operator()<4096>(); break;
                }
                
                //output data in csv format
                cout << align << ", " << time_taken / 2 << "\n";
                cout.flush();
            }
    }
    return 0;
}
