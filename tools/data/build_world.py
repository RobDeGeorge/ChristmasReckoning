#!/usr/bin/env python3
"""Build the world the Mission Control simulation runs on.

Inputs (fetched and pinned by fetch.py, cached in ~/.cache/christmasreckoning/raw):
  GHSL GHS-POP 2025 (30″ population grid), UN WPP 2024 (children by country), UN Household Size and
  Composition 2026 (children per household with children), Natural Earth countries,
  timezone-boundary-builder 2026f + IANA tzdata (UTC offsets on Christmas Eve).

For every ~1 km cell i we estimate households with a child under 18, H_i, and accumulate:
  S = Σ √(H_i·A_i)        Beardwood–Halton–Hammersley tour sum (metres; tour ≈ β·S)
  Q = Σ H_i^¾·A_i^¼       stop-and-go hop sum
by UTC-offset zone and by 0.25° block. Within each zone the blocks are put in Hilbert-curve order;
checkpoints along that order give the app cumulative fractions to animate deliveries, and a texture
encodes (zone, rank) per block so the globe can light up delivered regions with no per-frame uploads.

Outputs (committed):
  data/world.bin        binary world for the app (format: src/core/worlddata.cpp)
  data/delivery.png     1440×720 RGBA: R = zone+1, G·B = rank in zone, A = log household density
  data/world-report.json  totals and per-zone summary for review
"""
from __future__ import annotations

import argparse
import csv
import datetime as dt
import gzip
import hashlib
import json
import math
import pathlib
import struct
import sys
import time
import zipfile
import zoneinfo

import numpy as np
import openpyxl
import rasterio
import rasterio.features
import shapefile
from PIL import Image
from scipy import ndimage
from scipy.sparse import coo_matrix
from scipy.sparse.csgraph import connected_components, minimum_spanning_tree
from scipy.spatial import cKDTree

HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parents[1]
RAW = pathlib.Path.home() / ".cache" / "christmasreckoning" / "raw"
WORK = pathlib.Path.home() / ".cache" / "christmasreckoning" / "work"

EARTH_RADIUS_M = 6_371_008.8
BETA = 0.7124
BLOCK_DEG = 0.25
BLOCK_COLS, BLOCK_ROWS = 1440, 720
STRIP_ROWS = 120
MAX_CHECKPOINTS = 512
RESOLUTION_FACTORS = [(1, "30″ (~1 km)"), (2, "1′ (~2 km)"), (4, "2′ (~4 km)"), (10, "5′ (~9 km)"),
                      (30, "15′ (~28 km)"), (60, "30′ (~55 km)"), (120, "1° (~110 km)")]

# Natural Earth units without their own WPP entry, mapped to the WPP country that counts them.
NE_TO_WPP = {
    "ESB": "CYP", "WSB": "CYP", "CYN": "CYP", "CNM": "CYP", "SOL": "SOM", "KOS": "XKX", "USG": "CUB",
    "KAS": "IND", "SPI": "ARG", "BRT": "EGY", "ALA": "FIN", "ATC": "AUS", "IOA": "AUS", "HMD": "AUS",
    "NFK": "AUS", "CLP": "FRA", "ATF": "FRA", "UMI": "USA", "IOT": "GBR", "KAB": "KAZ", "PGA": None,
    "BJN": None, "SER": None, "SCR": None, "ATA": None, "SGS": None, "JEY": "CHI", "GGY": "CHI",
    "CXR": "AUS", "CCK": "AUS", "BVT": None, "SAH": "ESH",
}


def log(message: str) -> None:
    print(f"[{time.strftime('%H:%M:%S')}] {message}", file=sys.stderr, flush=True)


def parse_float(value) -> float | None:
    try:
        f = float(value)
    except (TypeError, ValueError):
        return None
    return f if math.isfinite(f) else None


# --- Tables --------------------------------------------------------------------------------------

def load_wpp(year: int) -> dict[str, dict]:
    """Population, children 0–17 and WPP parent region for every country/area in `year`."""
    path = RAW / "WPP2024_PopulationBySingleAgeSex_Medium_2024-2100.csv.gz"
    countries: dict[str, dict] = {}
    with gzip.open(path, "rt", encoding="utf-8-sig") as handle:
        for row in csv.DictReader(handle):
            if row["LocTypeName"] != "Country/Area" or int(row["Time"]) != year or not row["ISO3_code"]:
                continue
            iso = row["ISO3_code"]
            entry = countries.setdefault(iso, {"iso3": iso, "name": row["Location"], "parent": row["ParentID"],
                                               "population": 0.0, "children": 0.0})
            people = float(row["PopTotal"]) * 1000.0
            entry["population"] += people
            if int(row["AgeGrpStart"]) < 18:
                entry["children"] += people
    log(f"WPP {year}: {len(countries)} countries, {sum(c['population'] for c in countries.values()) / 1e9:.3f} bn people")
    return countries


def load_households() -> dict[str, dict]:
    """Per country, from its latest usable survey: children per household with children (κ),
    plus average household size and φ18 (share of households with a member under 18) for display.

    The database reports the mean number of under-15s among households with an under-15, and of
    under-20s among households with an under-20; κ for under-18s is interpolated between them
    (3/5 of the way, as ages 15–17 are three of the five years 15–19). Values from the same survey are
    always used together, and implausible entries (data errors) are skipped.
    """
    wb = openpyxl.load_workbook(RAW / "UNDESA_PD_2026_hh-size-composition.xlsx", read_only=True, data_only=True)
    ws = wb["HH size and composition 2026"]
    rows = ws.iter_rows(values_only=True)
    header = None
    for row in rows:
        if row and row[0] == "Country and area":
            header = list(row)
            break
    if header is None:
        raise SystemExit("household workbook: header row not found")
    names = {
        "iso": "ISO3 Code", "year": "Reference year", "region": "SDG region name",
        "size": "Average household size (number of members)", "phi": "Under 18 years (aged 0-17)",
        "k15": "Under age 15 years among households with at least one member under age 15 years",
        "k20": "Under age 20 years among households with at least one member under age 20 years",
    }
    col = {key: header.index(name) for key, name in names.items()}
    best: dict[str, dict] = {}
    for row in rows:
        if not row or not row[col["iso"]]:
            continue
        iso = row[col["iso"]]
        year = int(parse_float(row[col["year"]]) or 0)
        size = parse_float(row[col["size"]])
        phi = parse_float(row[col["phi"]])
        k15 = parse_float(row[col["k15"]])
        k20 = parse_float(row[col["k20"]])
        entry = best.setdefault(iso, {"region": row[col["region"]]})
        if k15 is not None and k20 is not None and 1.0 <= k15 <= 8 and 1.0 <= k20 <= 8 and year >= entry.get("kappaYear", -1):
            entry["kappa"] = k15 + 0.6 * (k20 - k15)
            entry["kappaYear"] = year
        if (size is not None and 1.5 <= size <= 15 and phi is not None and 0 < phi <= 100
                and year >= entry.get("sizeYear", -1)):
            entry["size"], entry["phi"], entry["sizeYear"] = size, phi / 100.0, year
    log(f"households: {len(best)} countries with records, {sum('kappa' in e for e in best.values())} with "
        f"children-per-household, {sum('size' in e for e in best.values())} with size and φ18")
    return best


def build_countries(wpp: dict[str, dict], households: dict[str, dict]) -> list[dict]:
    """WPP countries with κ (children per household with children); households H = children ÷ κ.

    Missing κ is imputed from the median of the country's WPP sub-region, else the world median.
    """
    by_parent: dict[str, list[dict]] = {}
    for iso, c in wpp.items():
        by_parent.setdefault(c["parent"], []).append(c)
    global_kappa = float(np.median([h["kappa"] for h in households.values() if "kappa" in h]))

    countries = []
    for iso, c in sorted(wpp.items(), key=lambda item: item[1]["name"]):
        h = households.get(iso, {})
        record = dict(c)
        kappa = h.get("kappa")
        imputed = kappa is None
        if imputed:
            vals = [households.get(n["iso3"], {}).get("kappa") for n in by_parent.get(c["parent"], [])
                    if n["iso3"] != iso]
            vals = [v for v in vals if v is not None]
            kappa = float(np.median(vals)) if vals else global_kappa
        record.update(childrenPerHousehold=kappa, imputed=imputed, surveyYear=int(h.get("kappaYear") or 0),
                      avgHouseholdSize=h.get("size", 0.0), shareWithChildren=h.get("phi", 0.0))
        record["childShare"] = c["children"] / c["population"] if c["population"] > 0 else 0.0
        record["hhPerPerson"] = record["childShare"] / kappa  # households with a child, per person
        countries.append(record)
    log(f"countries: {len(countries)} ({sum(c['imputed'] for c in countries)} with imputed κ); "
        f"world κ median {global_kappa:.2f}")
    return countries


def zone_offsets(tzids: list[str], year: int) -> dict[str, int]:
    """UTC offset in minutes at 22:00 local on 24 December of `year` (inside the delivery night)."""
    offsets = {}
    for tzid in tzids:
        local = dt.datetime(year, 12, 24, 22, 0, tzinfo=zoneinfo.ZoneInfo(tzid))
        offsets[tzid] = int(local.utcoffset().total_seconds() // 60)
    return offsets


def tzdata_version() -> str:
    try:
        first = pathlib.Path("/usr/share/zoneinfo/tzdata.zi").read_text().splitlines()[0]
        return first.replace("# version", "").strip()
    except OSError:
        return "unknown"


# --- Rasters -------------------------------------------------------------------------------------

def cached_raster(name: str, key: str, build) -> np.ndarray:
    WORK.mkdir(parents=True, exist_ok=True)
    path = WORK / f"{name}-{key}.npy"
    if path.exists():
        return np.load(path, mmap_mode="r")
    array = build()
    np.save(path, array)
    return np.load(path, mmap_mode="r")


def rasterize_countries(shape, transform, countries: list[dict]) -> tuple[np.ndarray, list[str]]:
    index = {c["iso3"]: i + 1 for i, c in enumerate(countries)}
    sf = shapefile.Reader(str(RAW / "ne_10m_admin_0_countries.zip"))
    shapes, unmatched = [], []
    for rec, shp in zip(sf.records(), sf.shapes()):
        code = rec["ISO_A3_EH"] if rec["ISO_A3_EH"] not in ("-99", "") else rec["ADM0_A3"]
        target = NE_TO_WPP.get(code, code) if code not in index else code
        if target is None or target not in index:
            unmatched.append(f"{rec['ADMIN']} ({code})")
            continue
        shapes.append((shp.__geo_interface__, index[target]))
    key = hashlib.sha1(json.dumps(sorted(str(s[1]) for s in shapes)).encode()).hexdigest()[:10]

    def build():
        log(f"rasterising {len(shapes)} country shapes at 30″ …")
        return rasterio.features.rasterize(shapes, out_shape=shape, transform=transform, fill=0, dtype="uint16")
    return cached_raster("countries", key, build), unmatched


def rasterize_timezones(shape, transform) -> tuple[np.ndarray, list[str]]:
    with zipfile.ZipFile(RAW / "timezones-now-2026f.geojson.zip") as zf:
        features = json.load(zf.open("combined-now.json"))["features"]
    tzids = [f["properties"]["tzid"] for f in features]

    def build():
        log(f"rasterising {len(features)} time zones at 30″ …")
        shapes = [(f["geometry"], i + 1) for i, f in enumerate(features)]
        return rasterio.features.rasterize(shapes, out_shape=shape, transform=transform, fill=0, dtype="uint8")
    return cached_raster("timezones", "2026f", build), tzids


def fill_nearest(labels: np.ndarray, need: np.ndarray) -> np.ndarray:
    """Give cells in `need` that have label 0 the label of the nearest labelled cell."""
    missing = need & (labels == 0)
    if not missing.any():
        return labels
    known = labels != 0
    if not known.any():
        return labels
    _, (iy, ix) = ndimage.distance_transform_edt(~known, return_indices=True)
    filled = labels.copy()
    filled[missing] = labels[iy[missing], ix[missing]]
    return filled


# --- Geometry ------------------------------------------------------------------------------------

def hilbert_index(x: np.ndarray, y: np.ndarray, order: int = 11) -> np.ndarray:
    """Distance along a Hilbert curve on a 2^order grid (vectorised xy→d)."""
    x = x.astype(np.int64).copy()
    y = y.astype(np.int64).copy()
    n = 1 << order
    d = np.zeros_like(x)
    s = n >> 1
    while s > 0:
        rx = ((x & s) > 0).astype(np.int64)
        ry = ((y & s) > 0).astype(np.int64)
        d += s * s * ((3 * rx) ^ ry)
        # rotate
        flip = ry == 0
        swap_x = np.where(flip & (rx == 1), n - 1 - x, x)
        swap_y = np.where(flip & (rx == 1), n - 1 - y, y)
        x = np.where(flip, swap_y, swap_x)
        y = np.where(flip, swap_x, swap_y)
        s >>= 1
    return d


def unit_vectors(lat_deg: np.ndarray, lon_deg: np.ndarray) -> np.ndarray:
    lat, lon = np.radians(lat_deg), np.radians(lon_deg)
    return np.column_stack([np.cos(lat) * np.cos(lon), np.cos(lat) * np.sin(lon), np.sin(lat)])


def great_circle_m(lat1, lon1, lat2, lon2) -> float:
    p1, p2 = math.radians(lat1), math.radians(lat2)
    dp, dl = p2 - p1, math.radians(lon2 - lon1)
    a = math.sin(dp / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dl / 2) ** 2
    return 2 * EARTH_RADIUS_M * math.asin(min(1.0, math.sqrt(a)))


def gap_distance(lat: np.ndarray, lon: np.ndarray) -> float:
    """Extra distance to hop between disconnected clusters of populated blocks.

    Minimum spanning tree over block centroids; edges no longer than ~1.5 block diagonals connect
    neighbouring blocks and are already covered by the BHH tour sum, so only the excess counts.
    """
    n = len(lat)
    if n < 2:
        return 0.0
    pts = unit_vectors(lat, lon)
    tree = cKDTree(pts)
    k = min(n - 1, 12)
    dist, idx = tree.query(pts, k + 1)
    rows = np.repeat(np.arange(n), k)
    cols = idx[:, 1:].ravel()
    w = dist[:, 1:].ravel()
    graph = coo_matrix((w + 1e-12, (rows, cols)), shape=(n, n)).tocsr()
    count, labels = connected_components(graph, directed=False)
    extra_rows, extra_cols, extra_w = [], [], []
    while count > 1:
        # Connect each component to its nearest point in any other component.
        for c in range(count):
            inside = np.where(labels == c)[0]
            outside = np.where(labels != c)[0]
            other = cKDTree(pts[outside])
            d, j = other.query(pts[inside], 1)
            best = int(np.argmin(d))
            extra_rows.append(int(inside[best]))
            extra_cols.append(int(outside[j[best]]))
            extra_w.append(float(d[best]) + 1e-12)
        add = coo_matrix((extra_w, (extra_rows, extra_cols)), shape=(n, n)).tocsr()
        graph = graph + add
        count, labels = connected_components(graph, directed=False)
    mst = minimum_spanning_tree(graph).tocoo()
    chord = mst.data
    metres = 2 * EARTH_RADIUS_M * np.arcsin(np.clip(chord / 2, 0, 1))
    mid_lat = np.radians((lat[mst.row] + lat[mst.col]) / 2)
    block_m = math.radians(BLOCK_DEG) * EARTH_RADIUS_M
    adjacent = 1.5 * block_m * np.sqrt(1 + np.cos(mid_lat) ** 2)
    return float(np.sum(np.maximum(0.0, metres - adjacent)))


# --- Main ----------------------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--year", type=int, default=2026, help="Christmas year for population and offsets")
    parser.add_argument("--out", type=pathlib.Path, default=REPO / "data")
    args = parser.parse_args()
    started = time.time()

    wpp = load_wpp(args.year)
    countries = build_countries(wpp, load_households())
    country_index = {c["iso3"]: i + 1 for i, c in enumerate(countries)}
    n_countries = len(countries) + 1
    hh_per_person = np.zeros(n_countries)
    child_share = np.zeros(n_countries)
    for i, c in enumerate(countries):
        hh_per_person[i + 1] = c["hhPerPerson"]
        child_share[i + 1] = c["childShare"]
    hh_per_person[0] = float(np.median(hh_per_person[1:]))
    child_share[0] = float(np.median(child_share[1:]))

    tif = RAW / "GHS_POP_E2025_GLOBE_R2023A_4326_30ss_V1_0.tif"
    ds = rasterio.open(tif)
    transform, shape = ds.transform, (ds.height, ds.width)
    x0, dx, y0, dy = transform.c, transform.a, transform.f, -transform.e
    country_raster, unmatched = rasterize_countries(shape, transform, countries)
    if unmatched:
        log("Natural Earth units without a WPP country (dropped): " + ", ".join(unmatched))
    tz_raster, tzids = rasterize_timezones(shape, transform)
    offsets_by_tz = zone_offsets(tzids, args.year)
    tz_offset = np.zeros(len(tzids) + 1, dtype=np.int32)
    for i, tzid in enumerate(tzids):
        tz_offset[i + 1] = offsets_by_tz[tzid]
    zone_list = sorted(set(offsets_by_tz.values()), reverse=True)
    zone_of_offset = {o: k for k, o in enumerate(zone_list)}
    tz_zone = np.full(len(tzids) + 1, -1, dtype=np.int32)
    for i, tzid in enumerate(tzids):
        tz_zone[i + 1] = zone_of_offset[offsets_by_tz[tzid]]
    n_zones = len(zone_list)
    log(f"{n_zones} UTC offsets on Christmas Eve {args.year}: "
        + ", ".join(f"{o / 60:+g}" for o in zone_list))

    lon_centres = x0 + (np.arange(shape[1]) + 0.5) * dx
    block_cols = (np.floor(((lon_centres + 180.0) % 360.0) / BLOCK_DEG).astype(np.int64)) % BLOCK_COLS

    # ---- Pass 1: GHSL totals per country (to rescale to WPP) ------------------------------------
    ghsl_by_country = np.zeros(n_countries)
    land_area_m2 = 0.0
    for row0 in range(0, shape[0], STRIP_ROWS):
        rows = min(STRIP_ROWS, shape[0] - row0)
        pop = ds.read(1, window=((row0, row0 + rows), (0, shape[1])))
        pop = np.where(np.isfinite(pop) & (pop > 0), pop, 0.0)
        cid = fill_nearest(np.asarray(country_raster[row0:row0 + rows]), pop > 0)
        ghsl_by_country += np.bincount(cid.ravel(), weights=pop.ravel(), minlength=n_countries)
        lat_top = y0 - np.arange(row0, row0 + rows) * dy
        area = (EARTH_RADIUS_M ** 2) * math.radians(dx) * (np.sin(np.radians(lat_top)) - np.sin(np.radians(lat_top - dy)))
        land_area_m2 += float(np.sum((np.asarray(country_raster[row0:row0 + rows]) > 0) * area[:, None]))
    rescale = np.ones(n_countries)
    missing = []
    for i, c in enumerate(countries):
        if ghsl_by_country[i + 1] > 0:
            rescale[i + 1] = c["population"] / ghsl_by_country[i + 1]
        elif c["population"] > 0:
            missing.append(f"{c['name']} ({c['population'] / 1e3:.0f}k)")
    log(f"pass 1: GHSL total {ghsl_by_country.sum() / 1e9:.3f} bn, unassigned {ghsl_by_country[0] / 1e6:.2f} M, "
        f"land area {land_area_m2 / 1e12:.1f} M km²")
    if missing:
        log("WPP countries with no GHSL population (not on the map): " + ", ".join(missing))

    # ---- Pass 2: households and tour sums --------------------------------------------------------
    n_blocks = BLOCK_COLS * BLOCK_ROWS
    key_index: dict[int, int] = {}
    acc = np.zeros((0, 8))  # P, C, H, S, Q, H·lat, H·lon, area
    dominant: dict[int, float] = {}  # (zone*n_blocks+block)*1024+country → H
    zone_totals = np.zeros((n_zones, 5))
    resolution = {k: 0.0 for k, _ in RESOLUTION_FACTORS}

    for row0 in range(0, shape[0], STRIP_ROWS):
        rows = min(STRIP_ROWS, shape[0] - row0)
        pop = ds.read(1, window=((row0, row0 + rows), (0, shape[1])))
        pop = np.where(np.isfinite(pop) & (pop > 0), pop, 0.0)
        if not pop.any():
            continue
        populated = pop > 0
        cid = fill_nearest(np.asarray(country_raster[row0:row0 + rows]), populated)
        tz = fill_nearest(np.asarray(tz_raster[row0:row0 + rows]), populated)
        lat_top = y0 - np.arange(row0, row0 + rows) * dy
        lat_c = lat_top - dy / 2
        area_row = (EARTH_RADIUS_M ** 2) * math.radians(dx) * (np.sin(np.radians(lat_top)) - np.sin(np.radians(lat_top - dy)))
        area = np.broadcast_to(area_row[:, None], pop.shape)

        people = pop * rescale[cid]
        households = people * hh_per_person[cid]
        children = people * child_share[cid]
        s_cell = np.sqrt(households * area)
        q_cell = households ** 0.75 * area ** 0.25

        # Resolution table: coarsen households and area into k×k cells.
        for k, _ in RESOLUTION_FACTORS:
            if k == 1:
                resolution[1] += float(s_cell.sum())
                continue
            pad_r = (-rows) % k
            pad_c = (-shape[1]) % k
            h = np.pad(households, ((0, pad_r), (0, pad_c)))
            a = np.pad(np.asarray(area), ((0, pad_r), (0, pad_c)))
            hk = h.reshape(h.shape[0] // k, k, h.shape[1] // k, k).sum(axis=(1, 3))
            ak = a.reshape(a.shape[0] // k, k, a.shape[1] // k, k).sum(axis=(1, 3))
            resolution[k] += float(np.sqrt(hk * ak)[hk > 0].sum())

        r_idx, c_idx = np.nonzero(populated)
        zones = tz_zone[tz[r_idx, c_idx]]
        if (zones < 0).any():
            raise SystemExit("populated cell without a time zone")
        brow = np.floor((90.0 - lat_c[r_idx]) / BLOCK_DEG).astype(np.int64).clip(0, BLOCK_ROWS - 1)
        block = brow * BLOCK_COLS + block_cols[c_idx]
        keys = zones.astype(np.int64) * n_blocks + block
        values = np.column_stack([
            people[r_idx, c_idx], children[r_idx, c_idx], households[r_idx, c_idx],
            s_cell[r_idx, c_idx], q_cell[r_idx, c_idx],
            households[r_idx, c_idx] * lat_c[r_idx], households[r_idx, c_idx] * lon_centres[c_idx],
            np.asarray(area)[r_idx, c_idx]])
        uniq, inverse = np.unique(keys, return_inverse=True)
        sums = np.zeros((len(uniq), values.shape[1]))
        np.add.at(sums, inverse, values)
        new = [int(u) for u in uniq if int(u) not in key_index]
        if new:
            start = len(key_index)
            for offset, u in enumerate(new):
                key_index[u] = start + offset
            acc = np.vstack([acc, np.zeros((len(new), acc.shape[1]))])
        rows_idx = np.array([key_index[int(u)] for u in uniq])
        acc[rows_idx] += sums
        np.add.at(zone_totals, zones, values[:, :5])

        ckeys = keys * 1024 + cid[r_idx, c_idx].astype(np.int64)
        cu, cinv = np.unique(ckeys, return_inverse=True)
        csum = np.bincount(cinv, weights=households[r_idx, c_idx])
        for u, w in zip(cu.tolist(), csum.tolist()):
            dominant[u] = dominant.get(u, 0.0) + w
        if row0 % (STRIP_ROWS * 20) == 0:
            log(f"pass 2: row {row0}/{shape[0]}, {len(key_index)} zone-blocks")
    ds.close()

    # ---- Per zone: order, checkpoints, gaps, transitions ------------------------------------------
    keys_arr = np.array(sorted(key_index, key=key_index.get), dtype=np.int64)
    zone_of_key = keys_arr // n_blocks
    block_of_key = keys_arr % n_blocks
    best_country: dict[int, tuple[int, float]] = {}
    zone_country_h = np.zeros((n_zones, n_countries))
    for ck, w in dominant.items():
        key, country = divmod(ck, 1024)
        z = key // n_blocks
        zone_country_h[z, country] += w
        if w > best_country.get(key, (0, -1.0))[1]:
            best_country[key] = (country, w)

    texture = np.zeros((BLOCK_ROWS, BLOCK_COLS, 4), dtype=np.uint8)
    texture_h = np.zeros(n_blocks)
    zones_out = []
    prev_lat, prev_lon = 90.0, 0.0
    for z in range(n_zones):
        sel = np.nonzero(zone_of_key == z)[0]
        if len(sel) == 0:
            continue
        rows = acc[sel]  # acc rows are in key_index insertion order, as is keys_arr
        H = rows[:, 2]
        keep = H > 0
        sel, rows, H = sel[keep], rows[keep], H[keep]
        if len(sel) == 0:
            continue
        blocks = block_of_key[sel]
        brow, bcol = blocks // BLOCK_COLS, blocks % BLOCK_COLS
        lat = rows[:, 5] / H
        lon = rows[:, 6] / H
        order = np.argsort(hilbert_index(bcol, brow))
        # Run the curve in whichever direction starts nearer to where Santa is.
        first, last = order[0], order[-1]
        if great_circle_m(prev_lat, prev_lon, lat[last], lon[last]) < great_circle_m(prev_lat, prev_lon, lat[first], lon[first]):
            order = order[::-1]
        rows, H, lat, lon, blocks, sel = rows[order], H[order], lat[order], lon[order], blocks[order], sel[order]
        transition = great_circle_m(prev_lat, prev_lon, lat[0], lon[0])
        prev_lat, prev_lon = float(lat[-1]), float(lon[-1])

        cum = np.cumsum(rows[:, :5], axis=0)
        totals = cum[-1]
        frac = cum / np.where(totals > 0, totals, 1)
        n = len(sel)
        if n <= MAX_CHECKPOINTS:
            picks = np.arange(n)
        else:
            qs = np.linspace(0, 1, MAX_CHECKPOINTS // 2 + 1)[1:]
            picks = np.unique(np.concatenate([np.searchsorted(frac[:, 3], qs), np.searchsorted(frac[:, 2], qs),
                                              [n - 1]]).clip(0, n - 1))
        checkpoints = []
        for i in picks:
            key = int(keys_arr[sel[i]])
            checkpoints.append({
                "children": float(frac[i, 1]), "households": float(frac[i, 2]), "path": float(frac[i, 3]),
                "stopgo": float(frac[i, 4]), "lat": float(lat[i]), "lon": float(lon[i]), "rank": int(i + 1),
                "country": max(0, best_country.get(key, (0, 0))[0] - 1)})
        checkpoints[-1].update(children=1.0, households=1.0, path=1.0, stopgo=1.0)

        # Texture: each block shows the zone with most households there.
        density = H / (rows[:, 7] / 1e6)  # households per km² of populated area
        for i, b in enumerate(blocks):
            if H[i] > texture_h[b]:
                texture_h[b] = H[i]
                r, c = divmod(int(b), BLOCK_COLS)
                texture[r, c] = (z + 1, i >> 8, i & 255,
                                 int(np.clip((math.log10(max(density[i], 1e-3)) + 2) * 42.5, 1, 255)))

        top = np.argsort(-zone_country_h[z])[:6]
        zones_out.append({
            "offset": zone_list[z], "population": float(totals[0]), "children": float(totals[1]),
            "households": float(totals[2]), "pathSum": float(totals[3]), "stopGoSum": float(totals[4]),
            "gap": gap_distance(lat, lon), "transition": transition, "cells": n, "checkpoints": checkpoints,
            "topCountries": [int(c) - 1 for c in top if c > 0 and zone_country_h[z, c] > 0][:4]})
        log(f"zone UTC{zone_list[z] / 60:+g}: {totals[2] / 1e6:8.1f} M households, {n:6d} blocks, "
            f"{len(checkpoints):3d} checkpoints, gap {zones_out[-1]['gap'] / 1e3:8.0f} km")

    # Re-number zones densely (offsets with no households are dropped) and fix texture zone ids.
    remap = {zone_of_offset[z["offset"]] + 1: i + 1 for i, z in enumerate(zones_out)}
    lut = np.zeros(256, dtype=np.uint8)
    for old, new in remap.items():
        lut[old] = new
    texture[..., 0] = lut[texture[..., 0]]

    write_outputs(args, countries, zones_out, resolution, land_area_m2, texture, started)
    return 0


def write_outputs(args, countries, zones, resolution, land_area_m2, texture, started) -> None:
    out = args.out
    out.mkdir(parents=True, exist_ok=True)
    sources = json.loads((HERE / "sources.json").read_text())["datasets"]
    total_h = sum(z["households"] for z in zones)
    resolution_rows = [{"label": label, "cellArcSeconds": 30 * k, "pathSumM": resolution[k]}
                       for k, label in RESOLUTION_FACTORS]
    resolution_rows.append({"label": "Uniform over all land", "cellArcSeconds": 0,
                            "pathSumM": math.sqrt(total_h * land_area_m2)})
    meta = {
        "dataYear": args.year,
        "tzVersion": f"tzdata {tzdata_version()}, boundaries 2026f",
        "landAreaM2": land_area_m2,
        "provenance": {
            "generated": dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "generator": "tools/data/build_world.py",
            "datasets": {k: {"title": v["title"], "license": v["license"], "sha256": v["sha256"]}
                         for k, v in sources.items()},
        },
        "countries": [{"iso3": c["iso3"], "name": c["name"], "population": c["population"],
                       "children": c["children"], "households": c["population"] * c["hhPerPerson"],
                       "childrenPerHousehold": c["childrenPerHousehold"],
                       "avgHouseholdSize": c["avgHouseholdSize"], "shareWithChildren": c["shareWithChildren"],
                       "surveyYear": c["surveyYear"], "imputed": c["imputed"]} for c in countries],
        "resolution": resolution_rows,
        "zones": [{"offset": z["offset"], "topCountries": z["topCountries"]} for z in zones],
    }
    blob = bytearray(b"CRWD")
    blob += struct.pack("<I", 1)
    js = json.dumps(meta, separators=(",", ":"), ensure_ascii=False).encode()
    blob += struct.pack("<I", len(js)) + js
    while len(blob) % 4:
        blob += b"\0"
    blob += struct.pack("<I", len(zones))
    for z in zones:
        blob += struct.pack("<iI7dII", z["offset"], z["cells"], z["population"], z["children"],
                            z["households"], z["pathSum"], z["stopGoSum"], z["gap"], z["transition"],
                            len(z["checkpoints"]), 0)
    for z in zones:
        for cp in z["checkpoints"]:
            blob += struct.pack("<6fIHH", cp["children"], cp["households"], cp["path"], cp["stopgo"],
                                cp["lat"], cp["lon"], cp["rank"], cp["country"], 0)
    (out / "world.bin").write_bytes(bytes(blob))
    Image.fromarray(texture, "RGBA").save(out / "delivery.png", optimize=True)

    report = {
        "dataYear": args.year,
        "population": sum(z["population"] for z in zones),
        "children": sum(z["children"] for z in zones),
        "households": total_h,
        "pathSumM": sum(z["pathSum"] for z in zones),
        "bhhTourKm": BETA * sum(z["pathSum"] for z in zones) / 1e3,
        "gapKm": sum(z["gap"] for z in zones) / 1e3,
        "landAreaKm2": land_area_m2 / 1e6,
        "resolution": resolution_rows,
        "zones": [{"utcOffsetHours": z["offset"] / 60, "households": z["households"], "children": z["children"],
                   "blocks": z["cells"], "gapKm": z["gap"] / 1e3, "transitionKm": z["transition"] / 1e3,
                   "topCountries": [countries[c]["name"] for c in z["topCountries"]]} for z in zones],
        "imputedCountries": [c["name"] for c in countries if c["imputed"]],
        "elapsedSeconds": round(time.time() - started),
    }
    (out / "world-report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n")
    log(f"wrote {out / 'world.bin'} ({len(blob) / 1024:.0f} KiB), delivery.png, world-report.json")
    log(f"children {report['children'] / 1e9:.3f} bn, households {total_h / 1e9:.3f} bn, "
        f"BHH tour {report['bhhTourKm']:.3g} km, gaps {report['gapKm']:.3g} km")


if __name__ == "__main__":
    sys.exit(main())
