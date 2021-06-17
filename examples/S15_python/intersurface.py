# Bimolecular reactions between molecules on different surfaces

__author__ = "Dilawar Singh"
__email__ = "dilawar.s.rajput@gmail.com"

import smoldyn


def test_getters():
    s = smoldyn.Simulation(low=[0, 0], high=[100, 100], types="r")
    A = s.addSpecies(
        "A", color={"all": "red"}, difc={"all": 1}, display_size=dict(all=1)
    )
    B = s.addSpecies(
        "B", color={"all": "green"}, difc={"all": 1}, display_size=dict(all=1)
    )
    C = s.addSpecies(
        "C", color={"all": "blue"}, difc={"all": 1}, display_size=dict(all=1)
    )

    p1 = smoldyn.Rectangle(corner=(0, 0), dimensions=[100], axis="+x")
    p2 = smoldyn.Rectangle(corner=(100, 0), dimensions=[100], axis="-x")
    p3 = smoldyn.Rectangle(corner=(0, 0), dimensions=[100], axis="+y")
    p4 = smoldyn.Rectangle(corner=(0, 100), dimensions=[100], axis="-y")
    walls = s.addSurface("walls", panels=[p1, p2, p3, p4])
    walls.setAction("both", [A, B, C], "reflect")

    r1 = smoldyn.Rectangle(corner=[49, 30], dimensions=[20], axis="+x", name="r1")
    t1 = smoldyn.Triangle(vertices=[[49, 50], [29, 70]], name="t1")
    left = s.addSurface(name="left", panels=[r1, t1])

    r1.neighbor = t1
    t1.neighbor = r1
    left.setAction("both", [A, B, C], "reflect")
    left.addMolecules((A, "up"), 20)

    r1 = smoldyn.Rectangle(corner=[50, 30], dimensions=[20], axis="+x", name="r1")
    t1 = smoldyn.Triangle(vertices=[[50, 30], [70, 10]], name="t1")
    right = s.addSurface(name="right", panels=[r1, t1])
    r1.neighbor = t1
    t1.neighbor = r1
    right.setAction("both", [A, B, C], "reflect")
    right.addMolecules((B, "up"), 20)

    rxn1 = s.addReaction(
        name="rxn1",
        subs=[(A, "up"), (B, "up")],
        prds=[(A, "up"), (C, "bsoln")],
        binding_radius=2,
    )
    rxn1.setIntersurface([1, 1])

    s.addGraphics("opengl_good")
    s = s.run(1000, dt=0.1)


def main():
    test_getters()


if __name__ == "__main__":
    main()
