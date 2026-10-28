static void HdGraphicsSettings_Draw(AeronUiContext* ui) {
 static bool save_failed;
 static const char* const choices[] = { "Original", "HD" };
 int selected = XwHdGraphics_Selected() ? 1 : 0;
 if (AeronUi_Selector(ui, "Models & textures", &selected, choices, 2))
  save_failed = !XwHdGraphics_Set(selected != 0);
 if (save_failed) AeronUi_Error(ui, "Could not save graphics choice. Please try again.");
 if (XwHdGraphics_Selected() != XwHdGraphics_Active())
  AeronUi_Help(ui, "Saved. Restart the game to apply models and textures.");
 else AeronUi_Help(ui, "Original game models or HD models and textures. Changes apply after restart.");
}
