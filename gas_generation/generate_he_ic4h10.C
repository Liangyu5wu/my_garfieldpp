#include <iostream>
#include "Garfield/FundamentalConstants.hh"
#include "Garfield/MediumMagboltz.hh"

using namespace Garfield;

int main(int argc, char* argv[]) {
    std::cout << "=== Generating He/iC4H10 Gas File ===\n";
    std::cout << "Gas mixture: 90% He + 10% iC4H10\n";
    std::cout << "Conditions: 1 atm, 20°C\n";
    std::cout << "Target: Gas gain ~2×10^5\n\n";
    
    // Setup the gas mixture: 90% He + 10% iC4H10
    MediumMagboltz gas("he", 90., "ic4h10", 10.);
    
    // Set standard conditions
    const double temperature = 293.15;  // 20°C
    const double pressure = AtmosphericPressure;  // 1 atm = 760 Torr
    
    gas.SetTemperature(temperature);
    gas.SetPressure(pressure);
    
    std::cout << "Temperature: " << temperature << " K\n";
    std::cout << "Pressure: " << pressure << " Torr\n\n";
    
    // Set the electric field range
    // For gas gain ~2×10^5, we need high fields (typically 50-200 kV/cm)
    // Including lower fields for complete coverage
    const size_t nE = 15;  // More points for better interpolation
    const double emin = 100.;      // 100 V/cm (low field region)
    const double emax = 100000.;   // 200 kV/cm (high gain region)
    
    // Use logarithmic spacing for better coverage
    constexpr bool useLog = true;
    
    gas.SetFieldGrid(emin, emax, nE, useLog);
    
    std::cout << "Electric field range: " << emin << " - " << emax << " V/cm\n";
    std::cout << "Number of E-field points: " << nE << "\n";
    std::cout << "Using logarithmic spacing: " << (useLog ? "Yes" : "No") << "\n\n";
    
    // Set number of collisions
    // He/iC4H10 mixture with quencher - use moderate value
    const int ncoll = 8;  // Good balance between accuracy and speed
    
    std::cout << "Starting Magboltz calculation...\n";
    std::cout << "Number of collisions: " << ncoll << " × 10^7\n";
    std::cout << "Please be patient...\n\n";
    
    // Generate the gas table
    gas.GenerateGasTable(ncoll);
    
    std::cout << "Magboltz calculation completed!\n";
    
    // Save the gas file
    const std::string filename = "he_90_ic4h10_10_1atm.gas";
    gas.WriteGasFile(filename);
    
    std::cout << "Gas file saved as: " << filename << "\n";
    
    // Print some basic information about the generated gas
    std::cout << "\n=== Gas Properties Summary ===\n";
    gas.PrintGas();
    
    // Check drift velocity at a few representative fields
    std::cout << "\n=== Sample Transport Properties ===\n";
    std::cout << "E [kV/cm]  Drift Vel [cm/μs]  Townsend α [1/cm]\n";
    
    std::vector<double> testFields = {1.0, 5.0, 10.0, 50.0, 100.0}; // kV/cm
    
    for (double field_kV : testFields) {
        double field_V = field_kV * 1000.0;  // Convert to V/cm
        
        // Get drift velocity
        double vx, vy, vz;
        if (gas.ElectronVelocity(field_V, 0, 0, 0, 0, 0, vx, vy, vz)) {
            vx *= 1000.0;  // Convert from cm/ns to cm/μs
            
            // Get Townsend coefficient (only 7 parameters)
            double alpha;
            if (gas.ElectronTownsend(field_V, 0, 0, 0, 0, 0, alpha)) {
                printf("%8.1f    %12.3f         %10.3e\n", field_kV, vx, exp(alpha));
            }
        }
    }
    
    std::cout << "\nNote: α values shown are exponential of stored ln(α)\n";
    std::cout << "For gas gain calculations, integrate α along drift path.\n";
    
    std::cout << "\n=== Generation Complete! ===\n";
    std::cout << "You can now use this gas file in your wire chamber simulation.\n";
    std::cout << "Update your wire_chamber.C to load: \"" << filename << "\"\n";
    
    return 0;
}