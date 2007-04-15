/*
 * $Id: Awk.h,v 1.21 2007-04-15 15:26:58 bacon Exp $
 *
 * {License}
 */

#ifndef _ASE_COM_AWK_H_
#define _ASE_COM_AWK_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "resource.h" 
#include "asecom.h"
#include <ase/awk/awk.h>
#include <ase/awk/val.h>
#include "awk_cp.h"

/////////////////////////////////////////////////////////////////////////////
// CAwk

class CAwk : 
	public IDispatchImpl<IAwk, &IID_IAwk, &LIBID_ASELib>, 
	public ISupportErrorInfo,
	/*public CComObjectRoot,*/
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAwk,&CLSID_Awk>,
	public IConnectionPointContainerImpl<CAwk>,
	public IProvideClassInfo2Impl<&CLSID_Awk, &DIID_IAwkEvents, &LIBID_ASELib>,
	public CProxyIAwkEvents< CAwk >

{
public:
	ase_awk_t* handle;
	int        option;
	int        errnum;
	ase_size_t errlin;
	ase_char_t errmsg[256];

	struct
	{
		struct
		{
			int parse;
			int run;
		} block;
		struct
		{
			int parse;
			int run;
		} expr;
		struct
		{
			int build;
			int match;
		} rex;
	} max_depth;

	IBuffer* read_src_buf;
	IBuffer* write_src_buf;
	ase_size_t read_src_pos;
	ase_size_t read_src_len;

	IBuffer* write_extio_buf;

	struct bfn_t
	{
		struct
		{
			TCHAR* ptr;
			size_t len;
		} name;
		size_t min_args;
		size_t max_args;
		struct bfn_t* next;
	} * bfn_list;

	BSTR entry_point;
	VARIANT_BOOL debug;
	VARIANT_BOOL use_longlong;
public:
	CAwk();
	~CAwk ();
	
BEGIN_COM_MAP(CAwk)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IAwk)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CAwk)
CONNECTION_POINT_ENTRY(DIID_IAwkEvents)
END_CONNECTION_POINT_MAP()

//DECLARE_NOT_AGGREGATABLE(CAwk) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_AWK)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IAwk
public:
	STDMETHOD(get_UseLongLong)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_UseLongLong)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_Debug)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Debug)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_EntryPoint)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_EntryPoint)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_MaxDepthForRexMatch)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_MaxDepthForRexMatch)(/*[in]*/ int newVal);
	STDMETHOD(get_MaxDepthForRexBuild)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_MaxDepthForRexBuild)(/*[in]*/ int newVal);
	STDMETHOD(get_MaxDepthForExprRun)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_MaxDepthForExprRun)(/*[in]*/ int newVal);
	STDMETHOD(get_MaxDepthForExprParse)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_MaxDepthForExprParse)(/*[in]*/ int newVal);
	STDMETHOD(get_MaxDepthForBlockRun)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_MaxDepthForBlockRun)(/*[in]*/ int newVal);
	STDMETHOD(get_MaxDepthForBlockParse)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_MaxDepthForBlockParse)(/*[in]*/ int newVal);
	STDMETHOD(get_ArgsToMain)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ArgsToMain)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_UseCrlf)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_UseCrlf)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_Nextofile)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Nextofile)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_StripSpaces)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_StripSpaces)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_StringBaseOne)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_StringBaseOne)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_SupportBlockless)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_SupportBlockless)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_SupportExtio)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_SupportExtio)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ConcatString)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ConcatString)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_IdivOperator)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_IdivOperator)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ShiftOperators)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ShiftOperators)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_VariableShading)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_VariableShading)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_UniqueFunction)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_UniqueFunction)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ExplicitVariable)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ExplicitVariable)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ImplicitVariable)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ImplicitVariable)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ErrorMessage)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_ErrorLine)(/*[out, retval]*/ int *pVal);
	STDMETHOD(get_ErrorCode)(/*[out, retval]*/ int *pVal);

	HRESULT __stdcall DeleteFunction (
		/*[in]*/ BSTR name, /*[out, retval]*/ VARIANT_BOOL* ret);
	HRESULT __stdcall AddFunction (
		/*[in]*/ BSTR name, /*[in]*/ int minArgs,
	       	/*[in]*/ int maxArgs, /*[out, retval]*/ VARIANT_BOOL* ret);

	HRESULT __stdcall Parse (/*[out, retval]*/ VARIANT_BOOL* ret);
	HRESULT __stdcall Run (/*[out, retval]*/ VARIANT_BOOL* ret);
};

#endif
