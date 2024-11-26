#pragma once
#include "player.h"

void create_session(Player leader);
void join_session(Player joiner, int sessionId);
void join_random_session(Player joiner);