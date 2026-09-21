#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

class AdvancedAviationECU {
private:
    const float sea_level_pressure_kPa = 101.3f;
    const float stoichiometric_ratio = 14.7f; // Teoreettinen ideaaliseos bensalle
    
    // Ilmailuturvallisuuden kriittiset rajat
    const float max_safe_egt_celsius = 850.0f;
    const float absolute_max_lean_afr = 16.0f;

public:
    AdvancedAviationECU() = default;

    // Laskee optimoidun AFR-arvon ottaen huomioon paineen, imulämpötilan (IAT) ja pakolämmön (EGT)
    float calculate_safety_afr(float current_pressure_kPa, float iat_celsius, float egt_celsius) {
        // 1. Anturivian varmistus (Sensor Fail-Safe)
        if (current_pressure_kPa <= 0.0f) {
            std::cout << "[VAROITUS] Paineanturivika! Käytetään rikastettua hätätila-AFR:ää.\n";
            return 13.0f; 
        }

        // 2. Korkeuskompensaatio (Ilmanpaine)
        float pressure_ratio = current_pressure_kPa / sea_level_pressure_kPa;
        float base_target_afr = stoichiometric_ratio + (1.0f - pressure_ratio) * 1.5f;

        // 3. Imuilman lämpötilakorjaus (IAT)
        // Standardilämpötila ilmailussa on 15 °C. Kylmempi ilma on tiheämpää -> vaatii rikkaampaa seosta (pienempi AFR).
        float iat_correction = (iat_celsius - 15.0f) * 0.015f;
        float adjusted_afr = base_target_afr + iat_correction;

        // 4. Mekaaninen turvaraja laihentamiselle
        if (adjusted_afr > absolute_max_lean_afr) {
            adjusted_afr = absolute_max_lean_afr;
        }

        // 5. Pakokaasun lämpötilavalvonta (EGT Thermal Protection)
        // Jos moottori käy liian kuumana, rikastetaan seosta välittömästi (pienennetään AFR:ää) sylinterien jäähdyttämiseksi.
        if (egt_celsius > max_safe_egt_celsius) {
            float overheat_delta = egt_celsius - max_safe_egt_celsius;
            float enrichment_factor = (overheat_delta / 50.0f) * 0.5f; // Rikastetaan 0.5 AFR per jokainen 50 °C ylitys
            
            std::cout << "[VAROITUS] EGT liian korkea (" << egt_celsius << "°C)! Pakkorikastus aktivoitu.\n";
            adjusted_afr -= enrichment_factor;
            
            // Varmistetaan, ettei suojarullaus rikastuta seosta liikaa (esim. alle tehorikkaan 12.0 AFR)
            if (adjusted_afr < 12.0f) adjusted_afr = 12.0f;
        }

        return adjusted_afr;
    }
};

int main() {
    AdvancedAviationECU ecu;
    std::cout << "--- TAURUS-AIR ADVANCED ECU FLIGHT SIMULATION ACTIVE ---\n\n";

    // TESTI 1: Normaali matkalento korkealla (Ohuempi ilma, kylmä imuilma, vakaa EGT)
    std::cout << "Testi 1: Matkalento 2500 metrissä\n";
    float afr1 = ecu.calculate_safety_afr(75.0f, -5.0f, 780.0f);
    std::cout << "-> Suositeltu seos: " << afr1 << " AFR\n\n";

    // TESTI 2: Kriittinen tilanne (Korkealla laihalla, mutta moottori alkaa ylikuumentua)
    std::cout << "Testi 2: Ylikuumenemistilanne lennolla (Korkea EGT)\n";
    float afr2 = ecu.calculate_safety_afr(75.0f, -5.0f, 890.0f);
    std::cout << "-> Suositeltu seos: " << afr2 << " AFR\n\n";

    // TESTI 3: Anturivika lennolla
    std::cout << "Testi 3: Vakava anturivika (Paineen lukema nolla)\n";
    float afr3 = ecu.calculate_safety_afr(0.0f, 15.0f, 600.0f);
    std::cout << "-> Suositeltu seos: " << afr3 << " AFR\n";

    return 0;
}
