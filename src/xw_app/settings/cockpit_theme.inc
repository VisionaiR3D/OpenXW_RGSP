/* Shared native cockpit skin. Metrics remain in Aeron's 1080p reference units. */
static void CommandCockpitTheme(AeronUiTheme* t) {
 #define CC(field,r,g,b) memcpy(t->field,(float[4]){r,g,b,1},sizeof t->field)
 CC(surface,.018f,.027f,.043f);
 CC(surface_border,.30f,.38f,.45f);
 CC(title_bar,.16f,.22f,.28f); CC(title_bar_low,.045f,.075f,.11f);
 CC(title_text,.69f,.88f,1.f); CC(text,.63f,.83f,.98f);
 CC(text_dim,.46f,.65f,.77f); CC(accent,1.f,.58f,.14f);
 CC(widget_bg,.065f,.105f,.15f); CC(widget_bg_low,.022f,.04f,.065f);
 CC(widget_bg_hot,.12f,.18f,.23f); CC(widget_border,.25f,.37f,.46f);
 CC(bevel_hi,.32f,.40f,.47f); CC(bevel_lo,.008f,.014f,.022f);
 CC(slider_track,.01f,.02f,.035f); CC(separator,.26f,.37f,.44f);
 #ifdef COMMAND_IMPERIAL
 CC(surface_border,.36f,.36f,.38f); CC(title_bar,.19f,.19f,.21f);
 CC(title_bar_low,.055f,.06f,.075f); CC(separator,.55f,.12f,.08f);
 #endif
 t->window_border_px=10; t->widget_border_px=3; t->bevel_px=4;
 t->corner_radius=2; t->widget_gradient=1;t->title_gradient=1;
 t->focus_px=0;memset(t->focus_outline,0,sizeof t->focus_outline);
 #undef CC
}
