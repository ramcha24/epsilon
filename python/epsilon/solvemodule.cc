#include <Python.h>

#include <setjmp.h>
#include <stdlib.h>

#include <glog/logging.h>

#include "epsilon/algorithms/prox_admm.h"
#include "epsilon/expression.pb.h"
#include "epsilon/expression/expression_util.h"
#include "epsilon/file/file.h"
#include "epsilon/parameters/local_parameter_service.h"
#include "epsilon/solver_params.pb.h"

// TODO(mwytock): Does failure handling need to be made threadsafe? Seems like
// making these threadlocal would do
static jmp_buf failure_buf;
static PyObject* SolveError;

extern "C" {

// TODO(mwytock): Need to set the appropriate exceptions when passed incorrect
// arguments.
static PyObject* prox_admm_solve(PyObject* self, PyObject* args) {
  const char* problem_str;
  const char* params_str;
  int problem_str_len, params_str_len;
  PyObject* data;

  // solve(problem_str, params_str, data)
  if (!PyArg_ParseTuple(
          args, "s#s#O",
          &problem_str, &problem_str_len,
          &params_str, &params_str_len,
          &data)) {
    return nullptr;
  }

  Problem problem;
  SolverParams params;
  if (!problem.ParseFromArray(problem_str, problem_str_len))
    return nullptr;
  if (!params.ParseFromArray(params_str, params_str_len))
    return nullptr;

  // NOTE(mwytock): References returned by PyDict_Next() are borrowed so no need
  // to Py_DECREF() them.
  Py_ssize_t pos = 0;
  PyObject* key;
  PyObject* value;
  while (PyDict_Next(data, &pos, &key, &value)) {
    const char* key_str = PyString_AsString(key);
    if (!key_str)
      return nullptr;

    const char* value_str = PyString_AsString(value);
    if (!value_str)
      return nullptr;

    {
      std::unique_ptr<file::File> f = file::Open(key_str, file::kWriteMode);
      f->Write(std::string(value_str, PyString_Size(value)));
      f->Close();
    }
  }

  ProxADMMSolver solver(
      problem, params,
      std::unique_ptr<ParameterService>(new LocalParameterService));

  if (!setjmp(failure_buf)) {
    // Standard execution path
    solver.Solve();
    std::string status_str = solver.status().SerializeAsString();

    // Get results
    PyObject* vars = PyDict_New();
    {
      LocalParameterService parameter_service;
      for (const Expression* expr : GetVariables(problem)) {
        const std::string& var_id = expr->variable().variable_id();
        uint64_t param_id = VariableParameterId(solver.problem_id(), var_id);
        Eigen::VectorXd x = parameter_service.Fetch(param_id);

        PyObject* val = PyString_FromStringAndSize(
            reinterpret_cast<const char*>(x.data()),
            x.rows()*sizeof(double));

        PyDict_SetItemString(vars, var_id.c_str(), val);
        Py_DECREF(val);
      }
    }

    PyObject* retval = Py_BuildValue("s#O", status_str.data(), status_str.size(), vars);
    Py_DECREF(vars);
    return retval;
  }

  PyErr_SetString(SolveError, "CHECK failed");
  return nullptr;
}

void handle_failure() {
  // TODO(mwytock): Dump stack trace here
  longjmp(failure_buf, 1);
}

static PyMethodDef SolveMethods[] = {
  {"prox_admm_solve", prox_admm_solve, METH_VARARGS,
   "Solve a problem with epsilon."},
  {nullptr, nullptr, 0, nullptr}
};

static bool initialized = false;
PyMODINIT_FUNC init_solve() {
  // TODO(mwytock): Increase logging verbosity based on environment variable
  if (!initialized) {
    const char* v = getenv("EPSILON_VLOG");
    if (v != nullptr)
      FLAGS_v = atoi(v);
    google::InitGoogleLogging("_solve");
    google::LogToStderr();
    google::InstallFailureFunction(&handle_failure);

    initialized = true;
  }

  PyObject* m = Py_InitModule("_solve", SolveMethods);
  if (m == nullptr)
    return;

  SolveError = PyErr_NewException("solve.error", nullptr, nullptr);
  Py_INCREF(SolveError);
  PyModule_AddObject(m, "error", SolveError);
}

}  // extern "C"
