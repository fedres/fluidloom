#pragma once

#include &lt;cmath&gt;
#include &lt;vector&gt;
#include &lt;utility&gt;

namespace fluidloom {
namespace validation {

/**
 * @brief Taylor-Green vortex analytical solution
 * 
 * 3D decaying vortex with known analytical solution for validation.
 * Initial condition: u = sin(x)cos(y)cos(z), v = -cos(x)sin(y)cos(z), w = 0
 */
class TaylorGreenSolution {
public:
    /**
     * @brief Compute velocity field at given point and time
     * @param x, y, z Spatial coordinates (normalized to [0, 2π])
     * @param t Time
     * @param Re Reynolds number
     * @param u, v, w Output velocity components
     */
    static void computeVelocity(
        double x, double y, double z, double t, double Re,
        double& u, double& v, double& w) {
        
        double nu = 1.0 / Re;
        double decay = std::exp(-2.0 * nu * t);
        
        u = std::sin(x) * std::cos(y) * std::cos(z) * decay;
        v = -std::cos(x) * std::sin(y) * std::cos(z) * decay;
        w = 0.0;
    }
    
    /**
     * @brief Compute kinetic energy at time t
     * @param t Time
     * @param Re Reynolds number
     * @return Total kinetic energy (normalized)
     */
    static double computeKineticEnergy(double t, double Re) {
        double nu = 1.0 / Re;
        double E0 = 1.0; // Initial energy (normalized)
        return E0 * std::exp(-4.0 * nu * t);
    }
    
    /**
     * @brief Compute energy decay rate
     * @param t Time
     * @param Re Reynolds number
     * @return dE/dt
     */
    static double computeEnergyDecayRate(double t, double Re) {
        double nu = 1.0 / Re;
        double E = computeKineticEnergy(t, Re);
        return -4.0 * nu * E;
    }
    
    /**
     * @brief Generate time series of kinetic energy
     * @param Re Reynolds number
     * @param t_final Final time
     * @param num_points Number of time points
     * @return Vector of (time, energy) pairs
     */
    static std::vector&lt;std::pair&lt;double, double&gt;&gt; computeEnergyDecay(
        double Re, double t_final, size_t num_points) {
        
        std::vector&lt;std::pair&lt;double, double&gt;&gt; result;
        result.reserve(num_points);
        
        for (size_t i = 0; i &lt; num_points; ++i) {
            double t = (t_final * i) / (num_points - 1);
            double E = computeKineticEnergy(t, Re);
            result.push_back({t, E});
        }
        
        return result;
    }
    
    /**
     * @brief Compute vorticity field
     * @param x, y, z Spatial coordinates
     * @param t Time
     * @param Re Reynolds number
     * @param omega_x, omega_y, omega_z Output vorticity components
     */
    static void computeVorticity(
        double x, double y, double z, double t, double Re,
        double& omega_x, double& omega_y, double& omega_z) {
        
        double nu = 1.0 / Re;
        double decay = std::exp(-2.0 * nu * t);
        
        // ω = ∇ × u
        omega_x = std::cos(x) * std::cos(y) * std::sin(z) * decay;
        omega_y = std::sin(x) * std::sin(y) * std::sin(z) * decay;
        omega_z = -2.0 * std::sin(x) * std::cos(y) * std::cos(z) * decay;
    }
    
    /**
     * @brief Compute enstrophy (integrated vorticity magnitude squared)
     * @param t Time
     * @param Re Reynolds number
     * @return Total enstrophy
     */
    static double computeEnstrophy(double t, double Re) {
        double nu = 1.0 / Re;
        double decay = std::exp(-4.0 * nu * t);
        // Analytical enstrophy for Taylor-Green vortex
        return 3.0 * decay; // Normalized
    }
};

} // namespace validation
} // namespace fluidloom
