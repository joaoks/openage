# Copyright 2026+ the openage authors. See copying.md for legal info.

"""
Tests for reading the .dat file formats.
"""

from __future__ import annotations

from openage.convert.value_object.init.game_version import GameEdition, GameVersion
from openage.testing.testing import assert_raises, assert_value, result

from .format_version import has_fixed_de2_drop_sites, has_recent_de2_format
from .research import Tech
from .unit import ActionUnit, UnitCommand

DE2_EDITION = GameEdition("Dummy DE2", "AOE2DE", "YES", [], [], {}, {}, [])
AOC_EDITION = GameEdition("Dummy AOC", "AOC", "YES", [], [], {}, {}, [])


def member_names(cls, game_version: GameVersion) -> list[str | None]:
    """
    Names of the members that cls reads for a game version.
    """
    return [member[1] for member in cls.get_data_format_members(game_version)]


def de2_formats() -> None:
    """
    The AoE2: DE .dat format is chosen by the .dat version.
    """
    old = GameVersion(edition=DE2_EDITION, datfile_version=(7, 7))
    mid = GameVersion(edition=DE2_EDITION, datfile_version=(7, 8))
    recent = GameVersion(edition=DE2_EDITION, datfile_version=(8, 9))
    newer = GameVersion(edition=DE2_EDITION, datfile_version=(10, 0))

    assert_value(
        [has_fixed_de2_drop_sites(v) for v in (old, mid, recent, newer)], [True, False, False, False]
    )
    assert_value([has_recent_de2_format(v) for v in (old, mid, recent, newer)], [False, False, True, True])

    # the format of other editions doesn't depend on the .dat version
    aoc = GameVersion(edition=AOC_EDITION)
    assert_value(has_fixed_de2_drop_sites(aoc), False)
    assert_value(has_recent_de2_format(aoc), False)

    # guessing the AoE2: DE format would misread the file
    with assert_raises(ValueError):
        result(has_recent_de2_format(GameVersion(edition=DE2_EDITION)))

    # builds before update 118476 store exactly 3 drop sites
    assert_value("drop_sites_count" in member_names(ActionUnit, old), False)
    assert_value("drop_sites_count" in member_names(ActionUnit, mid), True)

    # recent builds added members to unit commands and moved tech members behind the name
    for version, is_recent in ((old, False), (mid, False), (recent, True)):
        assert_value("unknown_de2" in member_names(UnitCommand, version), is_recent)
        tech_members = member_names(Tech, version)
        assert_value(tech_members.index("research_location_id") > tech_members.index("name"), is_recent)
