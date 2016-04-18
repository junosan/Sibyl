#!/bin/bash
find . -not \( -path ./core/Eigen -prune \) \( -name '*.h' -o -name '*.cc' \)
