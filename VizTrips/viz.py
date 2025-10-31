import xml.etree.ElementTree as ET
import numpy as np
from sklearn.manifold import MDS
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import argparse

def parse_args():
    parser = argparse.ArgumentParser(
        description="Visualize the trips made by a team in a TTP instance."
    )
    parser.add_argument("-i", "--instance", default="./example/CIRC10.xml", help="Path to the instance XML file (default: ./example/CIRC10.xml)")
    parser.add_argument("-s", "--solution", default="./example/CIRC10_TTP_OPT.xml", help="Path to the solution XML file (default: ./example/CIRC10_TTP_OPT.xml)")
    parser.add_argument("-o", "--output", default="output.pdf", help="Output PDF file name (default: output.pdf)")
    return parser.parse_args()


# === Parse instance.xml ===
def parse_instance(filename):
    tree = ET.parse(filename)
    root = tree.getroot()

    distances = {}
    teams = set()
    for d in root.findall(".//Distances/distance"):
        t1, t2, dist = int(d.attrib["team1"]), int(d.attrib["team2"]), float(d.attrib["dist"])
        distances[(t1, t2)] = dist
        distances[(t2, t1)] = dist
        teams.update([t1, t2])

    teams = sorted(list(teams))
    n = len(teams)
    dist_matrix = np.zeros((n, n))
    for i, t1 in enumerate(teams):
        for j, t2 in enumerate(teams):
            dist_matrix[i, j] = distances.get((t1, t2), 0)

    print("Inferring team coordinates using MDS...")
    mds = MDS(n_components=2, dissimilarity="precomputed", random_state=42)
    coords = mds.fit_transform(dist_matrix)
    team_coords = {team: tuple(coord) for team, coord in zip(teams, coords)}

    return team_coords, distances, teams


# === Parse solution.xml ===
def parse_solution(filename):
    tree = ET.parse(filename)
    root = tree.getroot()
    matches = []
    for m in root.findall(".//Games/ScheduledMatch"):
        matches.append({
            "home": int(m.attrib["home"]),
            "away": int(m.attrib["away"]),
            "slot": int(m.attrib["slot"])
        })
    return matches


# === Build per-team schedules ===
def build_schedules(matches):
    sched = {}
    for m in matches:
        home, away, slot = m["home"], m["away"], m["slot"]
        for t in (home, away):
            sched.setdefault(t, [])
        sched[home].append({"opponent": away, "slot": slot, "loc": "Home"})
        sched[away].append({"opponent": home, "slot": slot, "loc": "Away"})
    for s in sched.values():
        s.sort(key=lambda x: x["slot"])
    return sched


# === Compute trips ===
def compute_trips(team_id, schedule):
    trips, cur = [], []
    for m in schedule:
        if m["loc"] == "Away":
            cur.append(m["opponent"])
        else:
            if cur:
                trips.append([team_id] + cur + [team_id])
                cur = []
    if cur:
        trips.append([team_id] + cur + [team_id])
    return trips


# === Plot one team’s page ===
def plot_team_page(pdf, team_id, trips, team_coords, bounds):
    plt.figure(figsize=(6, 6))
    # All teams (background)
    for tid, (x, y) in team_coords.items():
        plt.scatter(x, y, color="gray", s=40, alpha=0.5)
        plt.text(x, y, str(tid), fontsize=8, ha="center", va="bottom", alpha=0.6)

    # Highlight team trips
    for trip in trips:
        xs, ys = zip(*[team_coords[t] for t in trip])
        plt.plot(xs, ys, marker="o", linewidth=2)

    plt.title(f"Team {team_id}")
    plt.xlim(bounds["x_min"], bounds["x_max"])
    plt.ylim(bounds["y_min"], bounds["y_max"])
    plt.grid(True)
    pdf.savefig(bbox_inches="tight")
    plt.close()


# === Main ===
def main():
    args = parse_args()

    team_coords, distances, teams = parse_instance(args.instance)
    matches = parse_solution(args.solution)
    schedules = build_schedules(matches)

    xs, ys = zip(*team_coords.values())
    bounds = {
        "x_min": min(xs) - 1,
        "x_max": max(xs) + 1,
        "y_min": min(ys) - 1,
        "y_max": max(ys) + 1,
    }

    with PdfPages(args.output) as pdf:
        for t in teams:
            trips = compute_trips(t, schedules.get(t, []))
            plot_team_page(pdf, t, trips, team_coords, bounds)

    print(f"Created vector PDF with all teams: {args.output}")


if __name__ == "__main__":
    main()

