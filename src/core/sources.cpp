// SPDX-License-Identifier: GPL-3.0-or-later
#include "sources.h"

namespace cr {

const std::vector<Source> &sources()
{
    static const std::vector<Source> list{
        // --- Datasets ---------------------------------------------------------------------------
        {"ghsl", "GHS-POP R2023A: GHS population grid, epoch 2025 (30″)",
         "Schiavina, Freire, Carioli & MacManus; European Commission JRC, 2023",
         "https://doi.org/10.2905/2FF68A52-5B5B-4A22-8F40-C41DA8332CFE",
         "Where people live, on a ~1 km grid. Rescaled to UN WPP 2024 country totals. CC BY 4.0."},
        {"wpp2024", "World Population Prospects 2024 (medium variant, single ages)",
         "United Nations, DESA, Population Division, 2024", "https://population.un.org/wpp/",
         "Population and children under 18 by country for the Christmas year. CC BY 3.0 IGO."},
        {"un-households", "Database on Household Size and Composition 2026",
         "United Nations, DESA, Population Division, 2026", "https://population.un.org/Household/",
         "Average household size and the share of households with a member under 18, from the latest "
         "census or survey per country (1959–2025); missing countries use their region's median. CC BY 3.0 IGO."},
        {"natural-earth", "Natural Earth 1:10m Admin 0 – Countries", "Natural Earth, v5.1",
         "https://www.naturalearthdata.com/", "Country borders for assigning population to countries. Public domain."},
        {"tz-boundary-builder", "timezone-boundary-builder, release 2026f",
         "Evan Siroky and contributors, from OpenStreetMap data, 2026",
         "https://github.com/evansiroky/timezone-boundary-builder",
         "Time zone borders. The derived zone tables are shared under the ODbL 1.0 as required."},
        {"tzdata", "IANA Time Zone Database", "IANA, 2026", "https://www.iana.org/time-zones",
         "Each zone's UTC offset on Christmas Eve, including southern-hemisphere daylight saving."},
        {"blue-marble", "Blue Marble: Next Generation, December", "NASA Earth Observatory, 2005",
         "https://visibleearth.nasa.gov/images/73909/", "Daytime Earth imagery (with December snow cover)."},
        {"black-marble", "Black Marble 2016", "NASA Earth Observatory, 2017",
         "https://earthobservatory.nasa.gov/features/NightLights", "City lights at night."},

        // --- Constants ----------------------------------------------------------------------------
        {"si", "The International System of Units (SI), 9th edition", "BIPM, 2019",
         "https://www.bipm.org/en/publications/si-brochure", "Exact values of c and k_B."},
        {"codata", "CODATA Recommended Values of the Fundamental Physical Constants: 2022", "NIST, 2024",
         "https://physics.nist.gov/cuu/Constants/", "Atomic mass unit."},
        {"cgpm-g0", "Standard acceleration of gravity, g₀ = 9.80665 m/s²", "3rd CGPM, 1901",
         "https://www.bipm.org/en/committees/cg/cgpm/3-1901/resolution-", "Converting accelerations to g."},
        {"iers", "IERS Conventions (2010), Technical Note 36", "IERS, 2010",
         "https://www.iers.org/IERS/EN/Publications/TechnicalNotes/tn36.html", "Earth's GM and rotation rate."},
        {"iugg-radius", "Geodetic Reference System 1980 (mean radius R₁)", "Moritz, IUGG, 2000",
         "https://doi.org/10.1007/s001900050278", "Earth's mean radius, 6,371.0088 km."},
        {"ussa1976", "U.S. Standard Atmosphere, 1976", "NOAA, NASA & USAF, 1976",
         "https://ntrs.nasa.gov/citations/19770009539", "Air density, temperature and speed of sound with altitude."},
        {"noaa-solar", "NOAA Solar Calculator equations", "NOAA Global Monitoring Laboratory (after Meeus, 1991)",
         "https://gml.noaa.gov/grad/solcalc/calcdetails.html", "Where the Sun is, for the day/night line."},

        // --- Route and scheduling -----------------------------------------------------------------
        {"bhh-constant", "The shortest path through many points", "Beardwood, Halton & Hammersley, 1959",
         "https://doi.org/10.1017/S0305004100034095",
         "Tour length ≈ β√(N·A), applied cell by cell on the 30″ grid. β ≈ 0.7124 (Percus & Martin, 1996, "
         "https://arxiv.org/abs/cond-mat/9607080)."},
        {"spacefilling", "Spacefilling curves and the planar travelling salesman problem", "Bartholdi & Platzman, 1989",
         "https://doi.org/10.1145/76359.76361", "Fast curve-based tours run about 25% longer than optimal (η ≈ 1.25)."},
        {"horn-1974", "Some simple scheduling algorithms", "Horn, 1974", "https://doi.org/10.1002/nav.3800210113",
         "Feasibility of jobs with release times and deadlines: the bottleneck run of time zones."},
        {"yds-1995", "A scheduling model for reduced CPU energy", "Yao, Demers & Shenker, 1995",
         "https://doi.org/10.1109/SFCS.1995.492493", "Energy-optimal speed for each time zone."},
        {"aasm-sleep", "Recommended amount of sleep for pediatric populations", "Paruthi et al., AASM, 2016",
         "https://doi.org/10.5664/jcsm.5866", "Children 6–12 need 9–12 h of sleep; teenagers 8–10 h."},

        // --- Aerothermodynamics -------------------------------------------------------------------
        {"hoerner", "Fluid-Dynamic Drag", "Hoerner, 1965", "https://archive.org/details/fluiddynamicdrag00hoer",
         "Blunt bodies at high speed have C_D ≈ 1."},
        {"sutton-graves", "A general stagnation-point convective-heating equation for arbitrary gas mixtures",
         "Sutton & Graves, NASA TR R-376, 1971", "https://ntrs.nasa.gov/citations/19720003329",
         "q = 1.7415×10⁻⁴ √(ρ/rₙ) v³ W/m². Fitted for ~2–15 km/s; used far beyond that here, so flagged."},
        {"nasa-real-gas", "Real gas effects", "NASA Glenn Research Center, Beginner's Guide to Aeronautics",
         "https://www.grc.nasa.gov/www/BGH/realgas.html",
         "Above ~800 K air molecules vibrate, then dissociate and ionise, so ideal-gas formulas fail."},
        {"meteor-ablation", "Meteor physics: the single-body ablation equations", "Ceplecha et al., 1998",
         "https://doi.org/10.1023/A:1005069928850", "Heat-transfer coefficient Λ for bodies entering the atmosphere."},
        {"water-hvap", "Enthalpy of vaporisation of water", "NIST Chemistry WebBook",
         "https://webbook.nist.gov/cgi/cbook.cgi?ID=C7732185", "≈2.26 MJ/kg plus heating: ≈2.6 MJ/kg for a body."},
        {"glasstone", "The Effects of Nuclear Weapons, 3rd edition", "Glasstone & Dolan, 1977",
         "https://www.osti.gov/biblio/6852629", "Blast overpressure and damage scaling."},

        // --- Yardsticks ---------------------------------------------------------------------------
        {"tnt-convention", "Ton of TNT equivalent = 4.184 GJ", "Convention (NIST SP 811, 2008)",
         "https://www.nist.gov/pml/special-publication-811", "Energy yardstick."},
        {"ei-review-2025", "Statistical Review of World Energy 2025", "Energy Institute, 2025",
         "https://www.energyinst.org/statistical-review", "World primary energy use in 2024: 592 EJ."},
        {"iau-2015", "IAU 2015 Resolution B3 (nominal solar values)", "International Astronomical Union, 2015",
         "https://arxiv.org/abs/1510.07674", "The Sun's luminosity, 3.828×10²⁶ W."},
        {"bullet", ".30-06 Springfield ballistics", "SAAMI specifications",
         "https://saami.org/technical-information/cartridge-chamber-drawings/", "A rifle bullet: ~940 m/s."},
        {"concorde", "Concorde: technical specifications", "British Airways / Aérospatiale",
         "https://www.heritage-concorde.com/concorde-facts", "Cruise at Mach 2.04, about 605 m/s."},
        {"iss", "International Space Station facts and figures", "NASA",
         "https://www.nasa.gov/international-space-station/space-station-facts-and-figures", "Orbits at ~7.66 km/s."},
        {"parker", "Fastest spacecraft speed: Parker Solar Probe", "Guinness World Records, 2024",
         "https://www.guinnessworldrecords.com/world-records/66135-fastest-spacecraft-speed",
         "692,017 km/h (192.2 km/s) at perihelion on 24 December 2024."},
        {"galactic-escape", "The escape velocity curve of the Milky Way", "Koppelman & Helmi, 2021",
         "https://arxiv.org/abs/2102.02211", "Escape speed from the Galaxy at the Sun's position: ~528 km/s."},

        // --- Living things ------------------------------------------------------------------------
        {"nasa-g-tolerance", "Human tolerance to acceleration (Bioastronautics Data Book)", "NASA SP-3006, 1973",
         "https://ntrs.nasa.gov/citations/19730006364",
         "About 5 g can be sustained for minutes; tens of g only for fractions of a second when restrained."},
        {"reindeer-mass", "Reindeer husbandry and body mass", "Reimers et al. / Nieminen; review in Rangifer",
         "https://septentrio.uit.no/index.php/rangifer", "Draught reindeer typically weigh 60–180 kg."},
        {"horsepower", "Horsepower", "Watt (1782); Stevenson & Wassersug, Nature 364, 1993",
         "https://doi.org/10.1038/364195a0", "1 hp = 745.7 W is a sustained draught horse; peaks reach ~15 hp."},
        {"weibel-2005", "Allometric scaling of maximal metabolic rate in mammals", "Weibel & Hoppeler, 2005",
         "https://doi.org/10.1242/jeb.01527", "Maximal power scales with body mass^0.87, used to estimate reindeer power."},
        {"usda-cookie", "Cookies, chocolate chip, commercially prepared, soft-type (FDC 172717)",
         "USDA FoodData Central", "https://fdc.nal.usda.gov/food-details/172717/nutrients",
         "444 kcal per 100 g, so a 15 g cookie is ~67 kcal."},
        {"compendium-met", "2011 Compendium of Physical Activities", "Ainsworth et al., 2011",
         "https://doi.org/10.1249/MSS.0b013e31821ece12", "Brisk walking is ~3.5 MET; kcal/min = MET·3.5·kg/200."},
        {"fina-pool", "Olympic swimming pool (FINA FR 2.1)", "World Aquatics", "https://www.worldaquatics.com/rules",
         "50 m × 25 m × 2 m = 2,500 m³."},
        {"iata-volumetric", "Volumetric (chargeable) weight, 6,000 cm³/kg", "IATA cargo convention",
         "https://www.iata.org/en/programs/cargo/", "Typical parcel density ≈ 167 kg/m³, for the size of the sack."},

        // --- The famous one ------------------------------------------------------------------------
        {"spy-1990", "Is there a Santa Claus? (the 'Santa physics' piece)", "Spy magazine, January 1990",
         "https://www.oocities.org/xuerd001/santa.htm",
         "The classic calculation: 91.8 million homes, 650 miles per second, 17,500 g. We redo it with "
         "real data and show where it went wrong."},
        {"assumption", "Assumption", "Christmas Reckoning", "",
         "A stated assumption you can change with the slider."},
    };
    return list;
}

const Source *findSource(std::string_view key)
{
    for (const Source &source : sources()) {
        if (source.key == key)
            return &source;
    }
    return nullptr;
}

} // namespace cr
