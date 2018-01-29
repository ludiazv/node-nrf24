#!/bin/bash
ssh -i ./debug_key -o StrictHostKeyChecking=no $1@$2
