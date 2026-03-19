#pragma once

/*
 * video.h  —  DS video output
 *
 * Top screen    (256x192): main game view, scaled from 160x192 TIA output
 * Bottom screen (256x192): score, room map, touch controls
 */

void video_init(void);
void video_render(void);
