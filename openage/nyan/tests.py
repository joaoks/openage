# Copyright 2026+ the openage authors. See copying.md for legal info.

"""
Tests for the nyan structures used by the converter.
"""

from openage.testing.testing import assert_raises, assert_value, result

from .nyan_structs import MemberOperator, MemberType, NyanMember, NyanMemberType


def number_operators() -> None:
    """
    int and float members only accept operands and operators that nyan accepts.
    """
    n_int = NyanMemberType(MemberType.INT)
    n_float = NyanMemberType(MemberType.FLOAT)

    # int members only accept whole numbers
    member = NyanMember("a", n_int, 2.0, MemberOperator.MULTIPLY)
    assert_value(member.value, 2)
    assert_value(member.dump_short(0), "a *= 2")

    member = NyanMember("a", n_int, 7.9, MemberOperator.ASSIGN)
    assert_value(member.dump_short(0), "a = 7")

    with assert_raises(ValueError):
        result(NyanMember("a", n_int, 1.1, MemberOperator.MULTIPLY))

    # ints can't be divided
    with assert_raises(TypeError):
        result(NyanMember("a", n_int, 2, MemberOperator.DIVIDE))

    # float members always get float literals
    member = NyanMember("b", n_float, 1.1, MemberOperator.MULTIPLY)
    assert_value(member.dump_short(0), "b *= 1.1")

    member = NyanMember("b", n_float, 100, MemberOperator.ASSIGN)
    assert_value(member.dump_short(0), "b = 100.0")

    member = NyanMember("b", n_float, 2, MemberOperator.DIVIDE)
    assert_value(member.dump_short(0), "b /= 2.0")
