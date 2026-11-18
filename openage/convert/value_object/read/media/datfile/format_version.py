# Copyright 2026+ the openage authors. See copying.md for legal info.

"""
Format differences between .dat files of the same game edition.
"""

from __future__ import annotations

import re
import typing

if typing.TYPE_CHECKING:
    from openage.convert.value_object.init.game_version import GameVersion


# Known .dat formats of AoE2: DE builds. Versions between the ones listed
# here have not been checked.
#
# Builds before update 118476 ("VER 7.7") store exactly 3 drop sites per unit.
DE2_FIXED_DROP_SITES_DATFILE_VERSION = (7, 7)
# Recent builds ("VER 8.9") added and moved unit and tech members.
DE2_RECENT_DATFILE_VERSION = (8, 9)


def read_datfile_version(data: bytes) -> tuple[int, int]:
    """
    Get the version from the version string at the start of a decompressed .dat file.

    >>> read_datfile_version(b"VER 7.7\\x00...")
    (7, 7)
    >>> read_datfile_version(b"VER 10.1...")
    (10, 1)
    """
    match = re.match(rb"VER (\d+)\.(\d+)", data[:8])
    if match is None:
        raise ValueError(f"unknown .dat version string: {data[:8]!r}")

    return int(match[1]), int(match[2])


def get_de2_datfile_version(game_version: GameVersion) -> tuple[int, int]:
    """
    Get the .dat version of an AoE2: DE game.
    """
    if game_version.edition.game_id != "AOE2DE":
        raise ValueError(f"not an AoE2: DE game: {game_version.edition.game_id}")

    if game_version.datfile_version is None:
        raise ValueError("the .dat format of AoE2: DE depends on the .dat version, which is unknown")

    return game_version.datfile_version


def has_fixed_de2_drop_sites(game_version: GameVersion) -> bool:
    """
    Check if the game is AoE2: DE with exactly 3 drop sites per unit in its .dat file.
    """
    return (
        game_version.edition.game_id == "AOE2DE"
        and get_de2_datfile_version(game_version) <= DE2_FIXED_DROP_SITES_DATFILE_VERSION
    )


def has_recent_de2_format(game_version: GameVersion) -> bool:
    """
    Check if the game is AoE2: DE with the .dat format of recent builds.
    """
    return (
        game_version.edition.game_id == "AOE2DE"
        and get_de2_datfile_version(game_version) >= DE2_RECENT_DATFILE_VERSION
    )
