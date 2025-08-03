#include <TApplication.h>
#include <TCanvas.h>
#include <TMarker.h>
#include <TROOT.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <cmath>
#include "Garfield/ComponentAnalyticField.hh"
#include "Garfield/DriftLineRKF.hh"
#include "Garfield/MediumMagboltz.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/TrackHeed.hh"
#include "Garfield/ViewDrift.hh"

using namespace Garfield;

bool readTransferFunction(Sensor& sensor) {
  std::ifstream infile;
  infile.open("mdt_elx_delta.txt", std::ios::in);
  if (!infile) {
    std::cerr << "Could not read chamber transfer function.\n";
    return false;
  }
  std::vector<double> times;
  std::vector<double> values;
  while (!infile.eof()) {
    double t = 0., f = 0.;
    infile >> t >> f;
    if (infile.eof() || infile.fail()) break;
    times.push_back(1.e3 * t);
    values.push_back(f);
  }
  infile.close();
  sensor.SetTransferFunction(times, values);
  return true;
}

int main(int argc, char* argv[]) {
  TApplication app("app", &argc, argv);
  
  std::cout << "=== Wire Chamber Simulation Debug ===\n";
  
  // Make a gas medium - BACK TO WORKING GAS
  std::cout << "Loading gas file...\n";
  MediumMagboltz gas;
  gas.LoadGasFile("ar_93_co2_7_3bar.gas");
  std::cout << "Gas loaded successfully.\n";
  
  std::cout << "Loading ion mobility...\n";
  gas.LoadIonMobility("IonMobility_Ar+_Ar.txt");
  // gas.LoadIonMobility("IonMobility_He+_He.txt");


  std::cout << "Ion mobility loaded.\n";
  
  // Make a component with analytic electric field
  std::cout << "Setting up electric field component...\n";
  ComponentAnalyticField cmp;
  cmp.SetMedium(&gas);
  std::cout << "Component created.\n";
  
  // Wire chamber parameters (same as before)
  const double cellSize = 1.4;               // 14mm cell size [cm]
  const double senseWireRadius = 10.e-4;     // 20μm sense wire [cm]
  const double fieldWireRadius = 20.e-4;     // 40μm field wire [cm]
  
  // Voltages (from MDT-like settings)
  const double senseVoltage = 2000.;
  const double fieldVoltage = 0.;            // Field wires grounded [V]
  
  std::cout << "Adding wires to geometry...\n";
  
  const double wireSpacing = cellSize / 2.0;
  const double halfwireSpacing = wireSpacing / 2.0;
  
  // Add sense wire at center
  cmp.AddWire(0.0, 0.0, senseWireRadius, senseVoltage, "s");
  std::cout << "Added sense wire at (0, 0)\n";
  
  // Add 12 field wires in square configuration around sense wire
  std::vector<std::pair<double, double>> fieldPositions = {
    {-wireSpacing, -wireSpacing}, // Bottom-left
    {-wireSpacing,  0.0},         // Left
    {-wireSpacing,  wireSpacing}, // Top-left
    { 0.0,         wireSpacing},  // Top
    { wireSpacing,  wireSpacing}, // Top-right
    { wireSpacing,  0.0},         // Right
    { wireSpacing, -wireSpacing}, // Bottom-right
    { 0.0,        -wireSpacing},   // Bottom
    { -halfwireSpacing,       -wireSpacing},
    { -halfwireSpacing,        wireSpacing},
    { halfwireSpacing,        -wireSpacing},
    { halfwireSpacing,         wireSpacing},
  };
  
  for (size_t i = 0; i < fieldPositions.size(); ++i) {
    std::string label = "field" + std::to_string(i);
    cmp.AddWire(fieldPositions[i].first, fieldPositions[i].second, 
                fieldWireRadius, fieldVoltage, label);
    std::cout << "Added field wire " << i << " at (" 
              << fieldPositions[i].first << ", " << fieldPositions[i].second << ")\n";
  }

  // Add boundary - SMALLER to ensure field coverage
  const double boundary = 1.8 * cellSize;  // Even smaller boundary
  cmp.AddPlaneX(-boundary, 0., "boundary");
  cmp.AddPlaneX( boundary, 0., "boundary");
  cmp.AddPlaneY(-boundary, 0., "boundary");
  cmp.AddPlaneY( boundary, 0., "boundary");
  
  std::cout << "Boundary set to ±" << boundary << " cm\n";

  // Make a sensor
  Sensor sensor(&cmp);
  sensor.AddElectrode(&cmp, "s");

  // Set the signal time window (MDT-like)
  const double tstep = 2.0/3.0;
  const double tmin = 0 * tstep;
  const unsigned int nbins = 3000;
  sensor.SetTimeWindow(tmin, tstep, nbins);

  // Set the delta response function
  if (!readTransferFunction(sensor)) return 0;
  sensor.ClearSignal();

  // Set up Heed (1 GeV muon)
  TrackHeed track(&sensor);
  track.SetParticle("pi-");
  track.SetMomentum(10.e9);  // 1 GeV/c
  // track.SetEnergy(1.e9);  // 1 GeV

  // RKF integration (adjusted for stability)
  DriftLineRKF drift(&sensor);
  drift.SetGainFluctuationsPolya(0., 20000.);  // Lower gain to avoid overflow
  std::cout << "Drift setup: gain = 20000\n";

  TCanvas* cD = nullptr;
  ViewDrift driftView;
  constexpr bool plotDrift = true;
  if (plotDrift) {
    cD = new TCanvas("cD", "", 600, 600);
    driftView.SetCanvas(cD);
    // Set smaller viewing area to focus on the detector
    driftView.SetArea(-2.0, -2.0, 2.0, 2.0);
    drift.EnablePlotting(&driftView);
    track.EnablePlotting(&driftView);
  }

  TCanvas* cS = nullptr;
  constexpr bool plotSignal = true;
  if (plotSignal) cS = new TCanvas("cS", "", 600, 600);

  // Track setup - LESS STEEP diagonal track to stay in detector
  const double x0 = -0.2;     // Start closer to center
  const double y0 = -1.0;     // Start closer to center  
  const double dx = 0.5;      // Gentler slope
  const double dy = 1.0;      // Direction: mainly upward
  const double dz = 0.0;      // No z component 
  
  // Normalize direction vector
  const double norm = sqrt(dx*dx + dy*dy + dz*dz);
  const double dx_norm = dx / norm;
  const double dy_norm = dy / norm;
  const double dz_norm = dz / norm;
  
  std::cout << "Track setup - DIAGONAL INCIDENT:\n";
  std::cout << "  Start: (" << x0 << ", " << y0 << ", 0)\n";
  std::cout << "  Direction: (" << dx_norm << ", " << dy_norm << ", " << dz_norm << ")\n";
  std::cout << "  Angle: " << atan2(dx_norm, dy_norm) * 180.0 / M_PI << " degrees from vertical\n";
  std::cout << "Particle: 1 GeV muon\n";
  
  const unsigned int nTracks = 1;
  
  for (unsigned int j = 0; j < nTracks; ++j) {
    std::cout << "\n=== Starting Track " << j+1 << " ===\n";
    sensor.ClearSignal();
    
    std::cout << "Creating diagonal muon track...\n";
    track.NewTrack(x0, y0, 0, 0, 0, 1, 0);
    // track.NewTrack(x0, y0, 0, dx_norm, dy_norm, dz_norm, 0);
    std::cout << "Track created successfully.\n";
    
    std::cout << "Getting ionization clusters...\n";
    auto clusters = track.GetClusters();
    std::cout << "Found " << clusters.size() << " clusters.\n";
    
    int totalElectrons = 0;
    for (const auto& cluster : clusters) {
      totalElectrons += cluster.electrons.size();
    }
    std::cout << "Total electrons to process: " << totalElectrons << "\n";
    
    if (totalElectrons == 0) {
      std::cout << "WARNING: No electrons generated!\n";
      continue;
    }
    
    std::cout << "Processing electrons (limited to 200 for better visualization)...\n";
    int processedElectrons = 0;
    const int maxElectrons = 200;  // Limit for cleaner visualization
    for (const auto& cluster : clusters) {
      for (const auto& electron : cluster.electrons) {
        if (processedElectrons >= maxElectrons) break;
        processedElectrons++;
        if (processedElectrons % 50 == 0) {
          std::cout << "  Processed " << processedElectrons << "/" << std::min(totalElectrons, maxElectrons) << " electrons\n";
        }
        drift.DriftElectron(electron.x, electron.y, electron.z, electron.t);
      }
      if (processedElectrons >= maxElectrons) break;
    }
    std::cout << "All electrons processed.\n";
    
    if (plotDrift) {
      std::cout << "Plotting drift lines...\n";
      cD->Clear();
      cD->SetTitle("Wire Chamber: Diagonal Incident Electron Drift");
      
      // Plot the cell structure with wires FIRST
      cmp.PlotCell(cD);
      
      // Then plot drift lines and track
      constexpr bool twod = true;
      constexpr bool drawaxis = true;
      driftView.Plot(twod, drawaxis);
      
      // Add manual markers for wire positions to make them visible
      cD->cd();
      // Draw sense wire
      auto* senseMark = new TMarker(0.0, 0.0, 29);  // Star marker
      senseMark->SetMarkerColor(kRed);
      senseMark->SetMarkerSize(2);
      senseMark->Draw();
      
      // Draw field wires
      for (size_t i = 0; i < fieldPositions.size(); ++i) {
        auto* fieldMark = new TMarker(fieldPositions[i].first, fieldPositions[i].second, 20);
        fieldMark->SetMarkerColor(kBlue);
        fieldMark->SetMarkerSize(1.5);
        fieldMark->Draw();
      }
      
      cD->Modified();
      cD->Update();
    }

    sensor.ConvoluteSignals();
    int nt = 0;
    if (!sensor.ComputeThresholdCrossings(-2., "s", nt)) continue;
    if (plotSignal) sensor.PlotSignal("s", cS);
  }

  app.Run(kTRUE);
}