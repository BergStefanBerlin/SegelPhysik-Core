---
description: "Use when creating or editing JSON files in SegelPhysik-Core. Keep model JSON readable with the agreed compact one-line layout without constraining its schema."
applyTo: "**/*.json"
---
# JSON-Darstellung

Diese Instruction legt ausschließlich die Textdarstellung fest. Sie friert
weder das JSON-Schema noch die fachliche Struktur ein; notwendige
Strukturänderungen bleiben möglich und richten sich nach den jeweiligen
Projektanforderungen.

- In Modellelementen stehen `darstellung`, `physik` und `setting` jeweils
  vollständig in einer Zeile.
- Verschachtelte Werte bleiben dabei Teil derselben Zeile. Dazu gehören
  beispielsweise `masse` und `verdrängung` innerhalb von `physik` sowie
  Quaternion- und Translationsarrays innerhalb von `setting`.
- `ende_vertex_indices` wird als vollständiges Array in einer Zeile
  dargestellt, zum Beispiel `"ende_vertex_indices": [49]`.
- Die einzelnen Eigenschaften eines Modellelements bleiben auf getrennten
  Zeilen. Nicht das gesamte Element zu einer Zeile zusammenziehen.
- Andere JSON-Objekte und Arrays dürfen zur Lesbarkeit mehrzeilig bleiben.
- Beim Formatieren keine Felder, Werte oder fachliche Struktur allein aus
  Layoutgründen hinzufügen, entfernen, umbenennen oder verschieben.
