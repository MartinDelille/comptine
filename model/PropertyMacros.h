#pragma once

#define PROPERTY_RW(Type, name, defaultValue)                           \
  Q_PROPERTY(Type name READ name WRITE set_##name NOTIFY name##Changed) \
public:                                                                 \
  Type name() const { return _##name; }                                 \
  Q_INVOKABLE void set_##name(Type value) {                             \
    if (_##name != value) {                                             \
      _##name = value;                                                  \
      emit name##Changed();                                             \
    }                                                                   \
  }                                                                     \
Q_SIGNALS:                                                              \
  void name##Changed();                                                 \
                                                                        \
private:                                                                \
  Type _##name = defaultValue;

#define PROPERTY_CONSTANT(Type, name, defaultValue) \
  Q_PROPERTY(Type name READ name CONSTANT)          \
public:                                             \
  Type name() const { return _##name; }             \
                                                    \
private:                                            \
  Type _##name = defaultValue;

#define PROPERTY_RO(Type, name)                        \
  Q_PROPERTY(Type name READ name NOTIFY name##Changed) \
public:                                                \
  Type name() const;                                   \
Q_SIGNALS:                                             \
  void name##Changed();                                \
                                                       \
private:

#define PROPERTY_RW_CUSTOM(Type, name, defaultValue)                    \
  Q_PROPERTY(Type name READ name WRITE set_##name NOTIFY name##Changed) \
public:                                                                 \
  Type name() const;                                                    \
  Q_INVOKABLE void set_##name(Type value);                              \
Q_SIGNALS:                                                              \
  void name##Changed();                                                 \
                                                                        \
private:                                                                \
  Type _##name = defaultValue;
