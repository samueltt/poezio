/* Copyright 2010-2011 Florent Le Coz <louiz@louiz.org> */

/* This file is part of Poezio. */

/* Poezio is free software: you can redistribute it and/or modify */
/* it under the terms of the zlib license. See the COPYING file. */

/** The poopt python3 module
**/

/* This file is a python3 module for poezio, used to replace some time-critical
python functions that are too slow. */

#define PY_SSIZE_T_CLEAN

#include "Python.h"

PyObject *ErrorObject;

/***
    The module functions
 ***/

/**
   cut_text: takes a string and returns a tuple of int.
   Each two int tuple is a line, represented by the ending position it (where it should be cut).
   Not that this position is calculed using the position of the python string characters,
   not just the individual bytes.
   For example, poopt_cut_text("vivent les réfrigérateurs", 6);
   will return [(0, 6), (7, 10), (11, 17), (17, 22), (22, 24)], meaning that the lines are
   "vivent", "les", "réfrig", "érateu" and "rs"
*/
PyDoc_STRVAR(poopt_cut_text_doc, "cut_text(text, width)\n\n\nReturn a list of two-tuple, the first int is the starting position of the line and the second is its end.");

static PyObject* poopt_cut_text(PyObject* self, PyObject* args)
{
    /* The list of tuples that we return */
    PyObject* retlist = PyList_New(0);

    /* Get the python arguments */
    const int width;
    const char* buffer;
    const size_t buffer_len;

    if (PyArg_ParseTuple(args, "is#", &width, &buffer, &buffer_len) == 0)
        return NULL;

    /* Pointer to the end of the string */
    const char* end = buffer + buffer_len;

    /* The position, considering UTF-8 chars (aka, the position in the
     * python string). This is used to determine the position in the python
     * string at which we should cut */
    int spos = 0;

    /* The start position (in the python-string) of the next line */
    int start_pos = 0;

    /* The position of the last space seen in the current line. This is used
     * to cut on spaces instead of cutting inside words, if possible (aka if
     * there is a space) */
    int last_space = -1;
    /* The number of columns taken by chars between start_pos and last_space */
    size_t cols_until_space = 0;

    /* The number of bytes consumed by mbrtowc. We advance the buffer ptr by this value */
    size_t consumed;

    /* Number of columns taken to display the current line so far */
    size_t columns = 0;

    /* The utf-8 char found by mbrtowc */
    wchar_t wc;

    while (buffer < end)
    {
        /* Special case to jump poezio special characters that are contained
         * in the python string, but should not be counted as chars, because
         * they will not be displayed. Those are the formatting chars (to
         * insert colors or things like that in the string) */
        if (*buffer == 25)   /* \x19 */
        {
            /* Jump everything until the end of this format marker, but
             * without increasing the number of columns of the current
             * line. Because these chars are not printed.  */
            while (buffer < end && *buffer != 'u' &&
                   *buffer != 'a' && *buffer != 'i' &&
                   *buffer != 'b' && *buffer != 'o' &&
                   *buffer != '}')
            {
                buffer++;
                spos++;
            }
            buffer++;
            spos++;
            continue;
        }
        /* Find the next unicode character (a wchar_t) in the string.  This
         * may consume from one to 4 bytes. */
        consumed = mbrtowc(&wc, buffer, end-buffer, NULL);
        if (consumed == 0)
            break ;
        else if ((size_t)-1 == consumed)
        {
            PyErr_SetString(PyExc_UnicodeError,
                            "mbrtowc returned -1: Invalid multibyte sequence.");
            return NULL;
        }
        else if ((size_t)-2 == consumed)
        {
            PyErr_SetString(PyExc_UnicodeError,
                            "mbrtowc returned -2: Could not parse a complete multibyte character.");
            return NULL;
        }

        buffer += consumed;
        /* Get the number of columns needed to display this character. May be 0, 1 or 2 */
        const size_t cols = wcwidth(wc);

        /* This is one condition to end the line: an explicit \n is found */
        if (wc == (wchar_t)'\n')
        {
            spos++;
            if (PyList_Append(retlist, Py_BuildValue("ii", start_pos, spos)) == -1)
                return NULL;
            /* And then initiate a new line */
            start_pos = spos;
            last_space = -1;
            columns = 0;
            continue ;
        }

        /* This is the second condition to end the line: we have consumed
         * enough characters to fill a whole line */
        if (columns + cols > width)
        {   /* If possible, cut on a space */
            if (last_space != -1)
            {
                if (PyList_Append(retlist, Py_BuildValue("ii", start_pos, last_space)) == -1)
                    return NULL;
                start_pos = last_space + 1;
                last_space = -1;
                columns -= (cols_until_space + 1);
            }
            else
            {
                /* Otherwise, cut in the middle of a word */
                if (PyList_Append(retlist, Py_BuildValue("ii", start_pos, spos)) == -1)
                    return NULL;
                start_pos = spos;
                columns = 0;
            }
        }
        /* We save the position of the last space seen in this line, and the
           number of columns we have until now. This helps us keep track of
           the columns to count when we will use that space as a cutting
           point, later */
        if (wc == (wchar_t)' ')
        {
            last_space = spos;
            cols_until_space = columns;
        }
        /* We advanced from one char, increment spos by one and add the
         * char's columns to the line's columns */
        columns += cols;
        spos++;
    }
    /* We are at the end of the string, append the last line, not finished */
    if (PyList_Append(retlist, Py_BuildValue("(i,i)", start_pos, spos)) == -1)
        return NULL;
    return retlist;
}

/***
    Module initialization. Just taken from the xxmodule.c template from the python sources.
 ***/
static PyTypeObject Str_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pooptmodule.Str",          /*tp_name*/
    0,                          /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    /* methods */
    0,                          /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_reserved*/
    0,                          /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash*/
    0,                          /*tp_call*/
    0,                          /*tp_str*/
    0,                          /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    0,                          /*tp_doc*/
    0,                          /*tp_traverse*/
    0,                          /*tp_clear*/
    0,                          /*tp_richcompare*/
    0,                          /*tp_weaklistoffset*/
    0,                          /*tp_iter*/
    0,                          /*tp_iternext*/
    0,                          /*tp_methods*/
    0,                          /*tp_members*/
    0,                          /*tp_getset*/
    0, /* see PyInit_xx */      /*tp_base*/
    0,                          /*tp_dict*/
    0,                          /*tp_descr_get*/
    0,                          /*tp_descr_set*/
    0,                          /*tp_dictoffset*/
    0,                          /*tp_init*/
    0,                          /*tp_alloc*/
    0,                          /*tp_new*/
    0,                          /*tp_free*/
    0,                          /*tp_is_gc*/
};

static PyObject *
null_richcompare(PyObject *self, PyObject *other, int op)
{
  Py_INCREF(Py_NotImplemented);
  return Py_NotImplemented;
}

static PyTypeObject Null_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "pooptmodule.Null",            /*tp_name*/
  0,                          /*tp_basicsize*/
  0,                          /*tp_itemsize*/
  /* methods */
  0,                          /*tp_dealloc*/
  0,                          /*tp_print*/
  0,                          /*tp_getattr*/
  0,                          /*tp_setattr*/
  0,                          /*tp_reserved*/
  0,                          /*tp_repr*/
  0,                          /*tp_as_number*/
  0,                          /*tp_as_sequence*/
  0,                          /*tp_as_mapping*/
  0,                          /*tp_hash*/
  0,                          /*tp_call*/
  0,                          /*tp_str*/
  0,                          /*tp_getattro*/
  0,                          /*tp_setattro*/
  0,                          /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
  0,                          /*tp_doc*/
  0,                          /*tp_traverse*/
  0,                          /*tp_clear*/
  null_richcompare,           /*tp_richcompare*/
  0,                          /*tp_weaklistoffset*/
  0,                          /*tp_iter*/
  0,                          /*tp_iternext*/
  0,                          /*tp_methods*/
  0,                          /*tp_members*/
  0,                          /*tp_getset*/
  0, /* see PyInit_xx */      /*tp_base*/
  0,                          /*tp_dict*/
  0,                          /*tp_descr_get*/
  0,                          /*tp_descr_set*/
  0,                          /*tp_dictoffset*/
  0,                          /*tp_init*/
  0,                          /*tp_alloc*/
  0, /* see PyInit_xx */      /*tp_new*/
  0,                          /*tp_free*/
  0,                          /*tp_is_gc*/
};


/* List of functions defined in the module */
static PyMethodDef poopt_methods[] = {
  {"cut_text", poopt_cut_text, METH_VARARGS, poopt_cut_text_doc},
  {}           /* sentinel */
};

PyDoc_STRVAR(module_doc,
             "This is a template module just for instruction. And poopt.");

/* Initialization function for the module (*must* be called PyInit_xx) */

static struct PyModuleDef pooptmodule = {
  PyModuleDef_HEAD_INIT,
  "poopt",
  module_doc,
  -1,
  poopt_methods,
  NULL,
  NULL,
  NULL,
  NULL
};

PyMODINIT_FUNC
PyInit_poopt(void)
{
  PyObject *m = NULL;

  /* Due to cross platform compiler issues the slots must be filled
   * here. It's required for portability to Windows without requiring
   * C++. */
  Null_Type.tp_base = &PyBaseObject_Type;
  Null_Type.tp_new = PyType_GenericNew;
  Str_Type.tp_base = &PyUnicode_Type;

  /* Finalize the type object including setting type of the new type
   * object; doing it here is required for portability, too. */
  /* if (PyType_Ready(&Xxo_Type) < 0) */
  /*     goto fail; */

  /* Create the module and add the functions */
  m = PyModule_Create(&pooptmodule);
  if (m == NULL)
    goto fail;

  /* Add some symbolic constants to the module */
  if (ErrorObject == NULL) {
    ErrorObject = PyErr_NewException("poopt.error", NULL, NULL);
    if (ErrorObject == NULL)
      goto fail;
  }
  Py_INCREF(ErrorObject);
  PyModule_AddObject(m, "error", ErrorObject);

  /* Add Str */
  if (PyType_Ready(&Str_Type) < 0)
    goto fail;
  PyModule_AddObject(m, "Str", (PyObject *)&Str_Type);

  /* Add Null */
  if (PyType_Ready(&Null_Type) < 0)
    goto fail;
  PyModule_AddObject(m, "Null", (PyObject *)&Null_Type);
  return m;
 fail:
  Py_XDECREF(m);
  return NULL;
}
