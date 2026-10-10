#include <thread>
#include <sched.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <cmath>
#include <array>
#include <cstdint>
#include <random>
#include <algorithm>

constexpr uint64_t number_of_sims_per_thread{100000};
constexpr uint8_t number_of_threads{10};
constexpr double error_tolerance{0.15};

constexpr inline double simulate(double& spot_price, double& strike_price,
                                 double& rate, double& time_to_expiry,
                                 double& current_sigma,
                                 uint64_t seed)
{
    std::philox4x32 rng(seed); 
    std::normal_distribution<double> normal(0.0, 1.0); 

    double payoff_sum{};
    for(auto i{0uz}; i < number_of_sims_per_thread; ++i)
    {
        const double z = normal(rng);

        const double expected_price =
            spot_price * std::exp( (rate - 0.5 * current_sigma * current_sigma)
                                   * time_to_expiry + current_sigma *
                                     std::sqrt(time_to_expiry) * z);
        payoff_sum += std::max(expected_price - strike_price, 0.0);
    } 
    return payoff_sum;
}

int main()
{
    std::array<std::jthread, number_of_threads> threads;

    std::cout << "Enter the contract informations in this order:\n"
              << "1. Spot Price\n2. Strike Price\n3. Call Option Price\n"
              << "4. Risk-Free Rate\n5. Time to expiry(in years)\n";
    double spot_price;
    double rate;
    double time_to_expiry;
    double strike_price;
    double contract_call_option_price;
    
    std::cin >> spot_price >> strike_price >> contract_call_option_price
             >> rate >> time_to_expiry;


    std::ofstream csv("prices.csv");
    if (!csv) 
    {
        std::cerr << "Could not create prices.csv\n";
        return 1;
    }

    csv << "sigma,estimated_call_price,contract_call_price\n";
    csv << std::setprecision(10);

    std::array<double, number_of_threads> payoff_sums;
    bool found = false;

    const auto start = std::chrono::steady_clock::now();        
    
    for(double current_sigma = 0.05; current_sigma <= 0.40; current_sigma += 0.05)
    {
        if(found)
            break;
        double payoff_sum{};

        for(auto i{0uz}; i < number_of_threads; ++i)
        {
            threads[i] = std::jthread([&, i]
            {
                           payoff_sums[i] = simulate(spot_price,
                                               strike_price,
                                               rate,
                                               time_to_expiry,
                                               current_sigma,
                                               42 + i);
            });

        }
        for(auto i{0uz}; i < number_of_threads; ++i)
            threads[i].join();
        for(auto i{0uz}; i < number_of_threads; ++i)
            payoff_sum += payoff_sums[i];

        double average_payoff = payoff_sum / (number_of_sims_per_thread * number_of_threads);

        double current_call_option_price = 
            std::exp( -rate * time_to_expiry ) * average_payoff;


        csv << current_sigma << ','
            << current_call_option_price << ','
            << contract_call_option_price << '\n';

        double price_difference = 
            std::abs(current_call_option_price - contract_call_option_price);

        if(current_call_option_price >= contract_call_option_price)
        {
            if(price_difference < error_tolerance)
            {
                std::cout << "The volatility factor used in this contract is "
                          << current_sigma << '\n';
                found = true;
                break;
            }
            else
            {
                while(current_sigma > 0.0)
                {
                    current_sigma -= 0.01;
                    payoff_sum = 0;
                    for(auto i{0uz}; i < number_of_threads; ++i)
                    {
                        threads[i] = std::jthread([&, i]
                                {
                                payoff_sums[i] = simulate(spot_price,
                                        strike_price,
                                        rate,
                                        time_to_expiry,
                                        current_sigma,
                                        42 + i);
                                });

                    }
                    for(auto i{0uz}; i < number_of_threads; ++i)
                        threads[i].join();
                    for(auto i{0uz}; i < number_of_threads; ++i)
                        payoff_sum += payoff_sums[i];

                    average_payoff = payoff_sum / (number_of_sims_per_thread * number_of_threads);

                    current_call_option_price = 
                        std::exp( -rate * time_to_expiry ) * average_payoff;

                    
                    csv << current_sigma << ','
                        << current_call_option_price << ','
                        << contract_call_option_price << '\n';

                    price_difference =
                        std::abs(current_call_option_price - contract_call_option_price);

                    if(price_difference < error_tolerance)
                    {
                        std::cout << "The volatility factor used in this contract is "
                                  << current_sigma << '\n';
                        found = true;
                        break;
                    }
                    if(current_call_option_price < contract_call_option_price)
                        break;
                }
                if(!found)
                {
                    std::cout << "We couldn't find the volatility factor used "
                              << "in this contract\n";
                    break;
                }
            }
        }
    }

    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = end - start;
    std::cout << "Simulation runtime: " << elapsed.count() << " seconds\n";

    csv.close();
    if(!found)
    {
        std::cout << "We couldn't find the volatility factor used in this "
                  << "contract.";
        return 0;
    }

    execl("/usr/bin/python3",
           "python3",
           "./plot.py",
           static_cast<char*>(nullptr));
    
    perror("Failed to run Python plot script");
    return 0;

}
