#!/bin/sh
git describe --exact-match --tags 2>/dev/null || git rev-parse --short HEAD
